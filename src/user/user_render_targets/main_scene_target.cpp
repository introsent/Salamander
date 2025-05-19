#include "main_scene_target.h"
#include "data_structures.h"
#include "depth_format.h"
#include "loaders/gltf_loader.h"
#include <iostream>
#include "deletion_queue.h"
#include "tiny_obj_loader.h"
#include "render_pass_executor.h"
#include "descriptors/descriptor_set_layout_builder.h"
#include "user_executors/main_scene_executor.h"

void MainSceneTarget::initialize(const SharedResources& shared) {
    m_shared = &shared;

    if (!m_shared) {
        throw std::runtime_error("m_shared is null after assignment");
    }
    if (!m_shared->frames) {
        throw std::runtime_error("frames pointer is null");
    }
    if (m_shared->frames->empty()) {
        throw std::runtime_error("frames vector is empty");
    }
    std::cout << "Frames size: " << m_shared->frames->size();

    createGBufferSampler();
    createGBufferAttachments();
    loadModel(MODEL_PATH);
    createBuffers();
    createDescriptors();
    createRenderingResources();
}

void MainSceneTarget::createGBufferSampler() {
    // Create sampler for G-buffer textures
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;


    if (vkCreateSampler(m_shared->context->device(), &samplerInfo, nullptr, &m_gBufferSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create G-buffer sampler");
    }

    VkDevice deviceCopy = m_shared->context->device();
    VkSampler samplerCopy = m_gBufferSampler;
    DeletionQueue::get().pushFunction("Sampler_" + std::to_string( TextureManager::getSamplerIndex()), [deviceCopy, samplerCopy]() {
        vkDestroySampler(deviceCopy, samplerCopy, nullptr);
        });
    TextureManager::incrementSamplerIndex();
}

void MainSceneTarget::createGBufferAttachments() {

    const auto& extent = m_shared->swapChain->extent();
    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    // Create albedo (RGBA8)
    {
        auto& a = m_shared->textureManager->createTexture(
            extent.width, extent.height,
            VK_FORMAT_R8G8B8A8_SRGB,
            usage,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VK_IMAGE_ASPECT_COLOR_BIT,
            false
        );
        m_albedoTexture.image = a.image;
        m_albedoTexture.allocation = a.allocation;
        m_albedoTexture.view  = a.view;
    }

    // Create normal (RG16F)
    {
        auto& n = m_shared->textureManager->createTexture(
            extent.width, extent.height,
            VK_FORMAT_R8G8B8A8_UNORM,
            usage,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VK_IMAGE_ASPECT_COLOR_BIT,
            false
        );
        m_normalTexture.image = n.image;
        m_normalTexture.allocation = n.allocation;
        m_normalTexture.view  = n.view;
    }

    // Create material params (RG8)
    {
        auto& p = m_shared->textureManager->createTexture(
            extent.width, extent.height,
            VK_FORMAT_R8G8_UNORM,
            usage,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VK_IMAGE_ASPECT_COLOR_BIT,
            false
        );
        m_paramTexture.image = p.image;
        m_paramTexture.allocation = p.allocation;
        m_paramTexture.view  = p.view;
    }
}


void MainSceneTarget::updateLightingDescriptors() {
    VkDescriptorImageInfo albedoInfo = {
        .sampler = m_gBufferSampler,
        .imageView = m_albedoTexture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkDescriptorImageInfo normalInfo = {
        .sampler = m_gBufferSampler,
        .imageView = m_normalTexture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkDescriptorImageInfo paramsInfo = {
        .sampler = m_gBufferSampler,
        .imageView = m_paramTexture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::vector<MainDescriptorManager::DescriptorUpdateInfo> lightingUpdates = {
        {
            .binding = 1,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = &albedoInfo,
            .descriptorCount = 1,
            .isImage = true
        },
        {
            .binding = 2,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = &normalInfo,
            .descriptorCount = 1,
            .isImage = true
        },
        {
            .binding = 3,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = &paramsInfo,
            .descriptorCount = 1,
            .isImage = true
        }
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_lightingDescriptorManager->updateDescriptorSet(i, lightingUpdates);
    }
}

void MainSceneTarget::createLightingPipeline() {
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkFormat colorFormat = m_shared->swapChain->format();
    VkPipelineRenderingCreateInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat = VK_FORMAT_UNDEFINED  // No depth in lighting pass
    };

    PipelineConfig pipelineConfig{
        .vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/lighting_vert.spv",  // Full-screen triangle
        .fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/lighting_frag.spv",  // Samples G-buffer

        .inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        },

        .viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1
        },

        .rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,  // No culling for full-screen pass
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f
        },

        .multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE
        },

        .depthStencil = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_EQUAL,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE
        },

        .colorBlendAttachments = { colorBlendAttachment },
        .colorBlending = colorBlending,

        .dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        },

        .rendering = renderingInfo,
    };

    m_lightingPipeline = std::make_unique<Pipeline>(
        m_shared->context,
        m_lightingDescriptorLayout->handle(),
        pipelineConfig
    );
}

void MainSceneTarget::createDepthPrepassPipeline() {
    // Unchanged from your original implementation
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.pColorAttachmentFormats = nullptr;
    renderingInfo.depthAttachmentFormat = m_shared->depthFormat;

    PipelineConfig config{};
    config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/depth_vert.spv";
    config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/depth_frag.spv";

    config.inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    config.viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    config.rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    config.multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    config.depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    config.colorBlendAttachments.clear();
    config.colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 0,
        .pAttachments = nullptr
    };

    config.dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    config.rendering = renderingInfo;

    m_depthPrepassPipeline = std::make_unique<Pipeline>(
        m_shared->context,
        m_gBufferDescriptorLayout->handle(),
        config
    );
}

void MainSceneTarget::createGBufferPipeline() {
    // Unchanged from your original implementation
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkFormat colorFormats[3] = {
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8_UNORM
    };

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 3;
    renderingInfo.pColorAttachmentFormats = colorFormats;
    renderingInfo.depthAttachmentFormat = m_shared->depthFormat;

    std::array<VkPipelineColorBlendAttachmentState, 3> colorBlendAttachments{};
    for (auto& attachment : colorBlendAttachments) {
        attachment.blendEnable = VK_FALSE;
        attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                    VK_COLOR_COMPONENT_G_BIT |
                                    VK_COLOR_COMPONENT_B_BIT |
                                    VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();

    PipelineConfig config{};
    config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/gbuffer_vert.spv";
    config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/gbuffer_frag.spv";

    config.inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    config.viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    config.rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    config.multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    config.depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    config.colorBlendAttachments = std::vector<VkPipelineColorBlendAttachmentState>(
        colorBlendAttachments.begin(), colorBlendAttachments.end()
    );
    config.colorBlending = colorBlending;

    config.dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    config.rendering = renderingInfo;

    m_gBufferPipeline = std::make_unique<Pipeline>(
        m_shared->context,
        m_gBufferDescriptorLayout->handle(),
        config
    );
}

void MainSceneTarget::createRenderingResources() {
    createDepthPrepassPipeline();
    createGBufferPipeline();
    createLightingPipeline();

    if (!m_shared || !m_shared->frames || m_shared->frames->empty()) {
        throw std::runtime_error("Frames not properly initialized!");
    }

    std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> depthViews;
    std::array<VkImage, MAX_FRAMES_IN_FLIGHT> depthImages;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        depthViews[i] = (*m_shared->frames)[i].depthTexture.view;
        depthImages[i] = (*m_shared->frames)[i].depthTexture.image;
    }

    MainSceneExecutor::Resources mainResources{
        .device =  m_shared->context->device(),
        .lightingPipeline = m_lightingPipeline->handle(),
        .lightingPipelineLayout = m_lightingPipeline->layout(),
        .depthPipeline = m_depthPrepassPipeline->handle(),
        .depthPipelineLayout = m_depthPrepassPipeline->layout(),
        .gBufferPipeline = m_gBufferPipeline->handle(),
        .gBufferPipelineLayout = m_gBufferPipeline->layout(),
        .vertexBufferAddress = m_deviceAddress,
        .indexBuffer = m_indexBuffer.handle(),
        .gBufferDescriptorSets = m_gBufferDescriptorManager->getDescriptorSets(),  // For depth and G-buffer
        .lightingDescriptorSets = m_lightingDescriptorManager->getDescriptorSets(),  // For lighting
        .indices = m_indices,
        .extent = m_shared->swapChain->extent(),
        .colorImageViews = m_shared->swapChain->imagesViews(),
        .depthImageViews = depthViews,
        .depthImages = depthImages,
        .gBufferAlbedoImage = m_albedoTexture.image,
        .gBufferAlbedoView = m_albedoTexture.view,
        .gBufferNormalImage = m_normalTexture.image,
        .gBufferNormalView = m_normalTexture.view,
        .gBufferParamsImage = m_paramTexture.image,
        .gBufferParamsView = m_paramTexture.view,
        .clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        .clearDepth = {.depthStencil = {1.0f, 0}},
        .swapChain = m_shared->swapChain,
        .primitives = m_primitives,
        .currentFrame = m_shared->currentFrame,
        .textureCount = static_cast<uint32_t>(m_materialTextures.size())
    };

    m_executor = std::make_unique<MainSceneExecutor>(mainResources);
}

void MainSceneTarget::createDescriptors() {
    auto textureCount = static_cast<uint32_t>(m_modelTextures.size());
    auto materialTextureCount = static_cast<uint32_t>(m_materialTextures.size());
    auto normalTextureCount = static_cast<uint32_t>(m_normalTextures.size());
    if (textureCount == 0) {
        throw std::runtime_error("No textures were loaded!");
    }

   //DescriptorSetLayoutBuilder depthLayoutBuilder(m_shared->context->device());
   //m_depthPrepassDescriptorLayout = depthLayoutBuilder
   //    .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
   //    .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
   //    .build();



    // G-buffer descriptor layout (for depth and G-buffer passes)
    DescriptorSetLayoutBuilder gBufferLayoutBuilder(m_shared->context->device());
    m_gBufferDescriptorLayout = gBufferLayoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, textureCount)
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, normalTextureCount)
        .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, materialTextureCount)
        .build();

    std::vector<VkDescriptorPoolSize> gBufferPoolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textureCount * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, normalTextureCount * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, materialTextureCount * MAX_FRAMES_IN_FLIGHT}
    };

    m_gBufferDescriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_gBufferDescriptorLayout->handle(),
        gBufferPoolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    // Lighting descriptor layout (for lighting pass)
    DescriptorSetLayoutBuilder lightingLayoutBuilder(m_shared->context->device());
    m_lightingDescriptorLayout = lightingLayoutBuilder
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Albedo
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Normal
        .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Params
        .build();

    std::vector<VkDescriptorPoolSize> lightingPoolSizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 * MAX_FRAMES_IN_FLIGHT}
    };

    m_lightingDescriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_lightingDescriptorLayout->handle(),
        lightingPoolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    // Update G-buffer descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_frameData[i].bufferInfo = {
            .buffer = m_uniformBuffers[i].handle(),
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        m_frameData[i].textureImageInfos.clear();
        for (const auto& texture : m_modelTextures) {
            m_frameData[i].textureImageInfos.push_back({
                .sampler = texture.sampler,
                .imageView = texture.view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }

        m_frameData[i].normalImageInfos.clear();
        for (const auto& texture : m_normalTextures) {
            m_frameData[i].normalImageInfos.push_back({
                .sampler = texture.sampler,
                .imageView = texture.view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }

        // Prepare material texture descriptors
        m_frameData[i].materialImageInfos.clear();
        for (const auto& texture : m_materialTextures) {
            m_frameData[i].materialImageInfos.push_back({
                .sampler = texture.sampler,
                .imageView = texture.view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }



        std::vector<MainDescriptorManager::DescriptorUpdateInfo> gBufferUpdates = {
            {
                .binding = 0,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .bufferInfo = &m_frameData[i].bufferInfo,
                .descriptorCount = 1,
                .isImage = false
            },
            {
                .binding = 1,
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .imageInfo = m_frameData[i].textureImageInfos.data(),
                .descriptorCount = static_cast<uint32_t>(m_frameData[i].textureImageInfos.size()),
                .isImage = true
            },
            {
                .binding = 2,
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .imageInfo = m_frameData[i].normalImageInfos.data(),
                .descriptorCount = static_cast<uint32_t>(m_frameData[i].normalImageInfos.size()),
                .isImage = true
            },
            {
                .binding = 5,
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .imageInfo = m_frameData[i].materialImageInfos.data(),
                .descriptorCount = static_cast<uint32_t>(m_frameData[i].materialImageInfos.size()),
                .isImage = true
            }
        };
        m_gBufferDescriptorManager->updateDescriptorSet(i, gBufferUpdates);
    }

    // Update lighting descriptor sets
    VkDescriptorImageInfo albedoInfo = {
        .sampler = m_gBufferSampler,
        .imageView = m_albedoTexture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkDescriptorImageInfo normalInfo = {
        .sampler = m_gBufferSampler,
        .imageView = m_normalTexture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkDescriptorImageInfo paramsInfo = {
        .sampler = m_gBufferSampler,
        .imageView = m_paramTexture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::vector<MainDescriptorManager::DescriptorUpdateInfo> lightingUpdates = {
        {
            .binding = 1,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = &albedoInfo,
            .descriptorCount = 1,
            .isImage = true
        },
        {
            .binding = 2,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = &normalInfo,
            .descriptorCount = 1,
            .isImage = true
        },
        {
            .binding = 3,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = &paramsInfo,
            .descriptorCount = 1,
            .isImage = true
        }
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_lightingDescriptorManager->updateDescriptorSet(i, lightingUpdates);
    }
}

void MainSceneTarget::render(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    updateUniformBuffers();
    VkMemoryBarrier2 memoryBarrier{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_UNIFORM_READ_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_UNIFORM_READ_BIT
    };

    VkDependencyInfo dependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &memoryBarrier
    };

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    m_executor->begin(commandBuffer, imageIndex);
    m_executor->execute(commandBuffer);
    m_executor->end(commandBuffer);
}


void MainSceneTarget::recreateSwapChain() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkWaitForFences(m_shared->context->device(), 1, &(*m_shared->frames)[i].inFlightFence, VK_TRUE, UINT64_MAX);
    }

    createGBufferAttachments();
    updateLightingDescriptors();

    std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> depthViews;
    std::array<VkImage, MAX_FRAMES_IN_FLIGHT> depthImages;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        depthViews[i] = (*m_shared->frames)[i].depthTexture.view;
        depthImages[i] = (*m_shared->frames)[i].depthTexture.image;
    }

    MainSceneExecutor::Resources mainResources{
        .device =  m_shared->context->device(),
        .lightingPipeline = m_lightingPipeline->handle(),
        .lightingPipelineLayout = m_lightingPipeline->layout(),
        .depthPipeline = m_depthPrepassPipeline->handle(),
        .depthPipelineLayout = m_depthPrepassPipeline->layout(),
        .gBufferPipeline = m_gBufferPipeline->handle(),
        .gBufferPipelineLayout = m_gBufferPipeline->layout(),
        .vertexBufferAddress = m_deviceAddress,
        .indexBuffer = m_indexBuffer.handle(),
        .gBufferDescriptorSets = m_gBufferDescriptorManager->getDescriptorSets(),  // For depth and G-buffer
        .lightingDescriptorSets = m_lightingDescriptorManager->getDescriptorSets(),  // For lighting
        .indices = m_indices,
        .extent = m_shared->swapChain->extent(),
        .colorImageViews = m_shared->swapChain->imagesViews(),
        .depthImageViews = depthViews,
        .depthImages = depthImages,
        .gBufferAlbedoImage = m_albedoTexture.image,
        .gBufferAlbedoView = m_albedoTexture.view,
        .gBufferNormalImage = m_normalTexture.image,
        .gBufferNormalView = m_normalTexture.view,
        .gBufferParamsImage = m_paramTexture.image,
        .gBufferParamsView = m_paramTexture.view,
        .clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        .clearDepth = {.depthStencil = {1.0f, 0}},
        .swapChain = m_shared->swapChain,
        .primitives = m_primitives,
        .currentFrame = m_shared->currentFrame,
        .textureCount = static_cast<uint32_t>(m_modelTextures.size())
    };

    m_executor = std::make_unique<MainSceneExecutor>(mainResources);
}

void MainSceneTarget::cleanup() {
    vkDeviceWaitIdle(m_shared->context->device());
    m_uniformBuffers.clear();
}

void MainSceneTarget::updateUniformBuffers() const {
    m_uniformBuffers[*(m_shared->currentFrame)].update(m_shared->context->device(), m_shared->swapChain->extent(), m_shared->camera);
}

void MainSceneTarget::loadModel(const std::string& modelPath) {
    GLTFModel gltfModel;
    if (!GLTFLoader::LoadFromFile(modelPath, gltfModel)) {
        throw std::runtime_error("Failed to load GLTF model");
    }

    m_modelTextures.clear();
    m_materialTextures.clear();  // Add a new vector for material textures
    std::unordered_map<std::string, size_t> texturePathToIndex;
    std::unordered_map<std::string, size_t> materialTexturePathToIndex;
    std::unordered_map<std::string, size_t> normalTexturePathToIndex;

    // First load all base color textures
    for (const auto& texInfo : gltfModel.textures) {
        std::string texturePath = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + texInfo.uri;
        auto it = texturePathToIndex.find(texturePath);
        if (it == texturePathToIndex.end()) {
            texturePathToIndex[texturePath] = m_modelTextures.size();
            m_modelTextures.push_back(m_shared->textureManager->loadTexture(texturePath));
        }
    }

    m_vertices = gltfModel.vertices;
    m_indices = gltfModel.indices;

    m_primitives.clear();
    m_primitives.reserve(gltfModel.primitives.size());

    // Now process all primitives and extract material information
    for (const auto& srcPrim : gltfModel.primitives) {
        uint32_t textureIndex = 0;
        uint32_t materialTextureIndex = 0;  // Default material texture index
        uint32_t normalTextureIndex = 0;  // Number of material textures

        if (srcPrim.materialIndex >= 0) {
            const auto& material = gltfModel.materials[srcPrim.materialIndex];

            // Handle base color texture
            if (material.baseColorTexture >= 0) {
                const auto& texInfo = gltfModel.textures[material.baseColorTexture];
                std::string texturePath = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + texInfo.uri;
                textureIndex = texturePathToIndex[texturePath];
            }

            if (material.normalTexture >= 0) {
                const auto& normalTexInfo = gltfModel.textures[material.normalTexture];
                std::string normTexturePath = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + normalTexInfo.uri;
                normalTextureIndex = texturePathToIndex[normTexturePath];

                auto it = normalTexturePathToIndex.find(normTexturePath);
                if (it == normalTexturePathToIndex.end()) {
                    normalTexturePathToIndex[normTexturePath ] = m_normalTextures.size();
                    m_normalTextures.push_back(m_shared->textureManager->loadTexture(normTexturePath, VK_FORMAT_R8G8B8A8_UNORM));
                    normalTextureIndex  = static_cast<uint32_t>(m_normalTextures.size() - 1);
                } else {
                    normalTextureIndex = static_cast<uint32_t>(it->second);
                }
            }
            else
            {
                normalTextureIndex = UINT32_MAX;
            }

            // Handle metallic-roughness texture
            if (material.metallicRoughnessTexture >= 0) {
                const auto& mrTexInfo = gltfModel.textures[material.metallicRoughnessTexture];
                std::string mrTexturePath = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + mrTexInfo.uri;

                auto it = materialTexturePathToIndex.find(mrTexturePath);
                if (it == materialTexturePathToIndex.end()) {
                    materialTexturePathToIndex[mrTexturePath] = m_materialTextures.size();
                    m_materialTextures.push_back(m_shared->textureManager->loadTexture(mrTexturePath));
                    materialTextureIndex = static_cast<uint32_t>(m_materialTextures.size() - 1);
                } else {
                    materialTextureIndex = static_cast<uint32_t>(it->second);
                }
            } else {
                // Create and add a default metallic-roughness texture based on the material factors
                materialTextureIndex = createDefaultMaterialTexture(material.metallicFactor, material.roughnessFactor);
            }
        }

        GLTFPrimitiveData primitive{
            .indexOffset = srcPrim.indexOffset,
            .indexCount = srcPrim.indexCount,
            .materialIndex = textureIndex,
            .metalRoughTextureIndex = materialTextureIndex,
            .normalTextureIndex = normalTextureIndex
        };
        m_primitives.push_back(primitive);
    }
}

uint32_t MainSceneTarget::createDefaultMaterialTexture(float metallicFactor, float roughnessFactor) {
    std::string key = "default_" + std::to_string(metallicFactor) + "_" + std::to_string(roughnessFactor);

    // Check if we already have this default texture
    for (size_t i = 0; i < m_defaultMaterialKeys.size(); i++) {
        if (m_defaultMaterialKeys[i] == key) {
            return static_cast<uint32_t>(i);
        }
    }

    unsigned char data[4] = {
        0,                                          // R - unused
        static_cast<unsigned char>(roughnessFactor * 255), // G - roughness
        static_cast<unsigned char>(metallicFactor * 255),  // B - metallic
        255                                         // A - opacity
    };

    // Create the texture and add it to our list
    ManagedTexture defaultMat = m_shared->textureManager->createTexture(data, 1, 1, 4);
    m_materialTextures.push_back(defaultMat);
    m_defaultMaterialKeys.push_back(key);

    return static_cast<uint32_t>(m_materialTextures.size() - 1);
}


void MainSceneTarget::createBuffers() {
    m_ssboBuffer = SSBOBuffer(m_shared->bufferManager, m_shared->commandManager, m_shared->allocator,
                              m_vertices.data(), sizeof(Vertex) * m_vertices.size());

    m_deviceAddress = m_ssboBuffer.getDeviceAddress(m_shared->context->device());

    m_indexBuffer = IndexBuffer(m_shared->bufferManager, m_shared->commandManager, m_shared->allocator, m_indices);

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    m_uniformBuffers.clear();
    m_uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_uniformBuffers.emplace_back(m_shared->bufferManager, m_shared->allocator, bufferSize);
    }
}