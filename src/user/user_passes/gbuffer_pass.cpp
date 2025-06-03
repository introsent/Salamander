#include "gbuffer_pass.h"

#include "config.h"
#include "pipeline.h"
#include "descriptors/descriptor_set_layout_builder.h"
#include "image_transition_manager.h"
#include "loaders/gltf_loader.h"
#include "shared/scene_data.h"
#include "target/render_target.h"

void GBufferPass::initialize(const RenderTarget::SharedResources& shared,
                             MainSceneGlobalData& globalData,
                             PassDependencies& dependencies) {
    m_shared = &shared;
    m_globalData = &globalData;
    m_dependencies = &dependencies;
    
    createAttachments();
    createDescriptors();
    createPipeline();
}

void GBufferPass::cleanup() {
    m_pipeline.reset();
    m_descriptorManager.reset();
    m_descriptorLayout.reset();
    // Textures cleaned up by texture manager
}

void GBufferPass::recreateSwapChain() {
    createAttachments();
}

void GBufferPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) {
    // Transition attachments
    ImageTransitionManager::transitionColorAttachment(
        cmd, m_albedoTexture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    ImageTransitionManager::transitionColorAttachment(
        cmd, m_normalTexture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    ImageTransitionManager::transitionColorAttachment(
        cmd, m_paramTexture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    
    // Set up attachments
    std::array<VkRenderingAttachmentInfo, 3> colorAttachments = {{
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = m_albedoTexture.view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
        },
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = m_normalTexture.view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color = {0.0f, 0.0f, 0.0f, 0.0f}}
        },
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = m_paramTexture.view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color = {0.0f, 0.0f, 0.0f, 0.0f}}
        }
    }};


    VkRenderingAttachmentInfo depthAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = m_dependencies->depthTexture->view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE
    };


    VkRenderingInfo renderInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = {{0, 0}, m_shared->swapChain->extent()},
        .layerCount = 1,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
        .pColorAttachments = colorAttachments.data(),
        .pDepthAttachment = &depthAttachment
    };
    
    vkCmdBeginRendering(cmd, &renderInfo);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());
    
    // Set dynamic viewport/scissor
    VkViewport viewport = {
        0.0f, 0.0f,
        static_cast<float>(m_shared->swapChain->extent().width),
        static_cast<float>(m_shared->swapChain->extent().height),
        0.0f, 1.0f
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    
    VkRect2D scissor = {{0, 0}, m_shared->swapChain->extent()};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    
    // Bind descriptor set
    vkCmdBindDescriptorSets(
        cmd, 
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipeline->layout(),
        0, 1,
        &m_descriptorManager->getDescriptorSets()[frameIndex],
        0, nullptr
    );
    
    // Bind index buffer
    vkCmdBindIndexBuffer(cmd, m_globalData->indexBuffer.handle(), 0, VK_INDEX_TYPE_UINT32);
    
    // Draw all primitives
    for (const auto& primitive : m_globalData->primitives) {
        PushConstants pc = {
            .vertexBufferAddress = m_globalData->vertexBufferAddress,
            .baseColorTextureIndex = primitive.materialIndex,
            .metalRoughTextureIndex = primitive.metalRoughTextureIndex,
            .normalTextureIndex = primitive.normalTextureIndex,
            .textureCount = static_cast<uint32_t>(m_globalData->modelTextures.size()),
            .modelScale = globalScale
        };
        vkCmdPushConstants(
            cmd, m_pipeline->layout(), 
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pc
        );
        vkCmdDrawIndexed(
            cmd, primitive.indexCount, 1, 
            primitive.indexOffset, 0, 0
        );
    }
    
    vkCmdEndRendering(cmd);
    
    // Transition attachments to shader read
    ImageTransitionManager::transitionToShaderRead(
        cmd, m_albedoTexture.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    ImageTransitionManager::transitionToShaderRead(
        cmd, m_normalTexture.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    ImageTransitionManager::transitionToShaderRead(
        cmd, m_paramTexture.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

}

void GBufferPass::createPipeline() {
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    // Attachment formats
    std::array<VkFormat, 3> colorFormats = {
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R8G8_UNORM
    };
    
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = colorFormats.size();
    renderingInfo.pColorAttachmentFormats = colorFormats.data();
    renderingInfo.depthAttachmentFormat = m_shared->depthFormat;
    
    // Blend states (no blending)
    std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachments{};
    for (auto& attachment : blendAttachments) {
        attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                   VK_COLOR_COMPONENT_G_BIT | 
                                   VK_COLOR_COMPONENT_B_BIT | 
                                   VK_COLOR_COMPONENT_A_BIT;
    }
    
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
        blendAttachments.begin(), blendAttachments.end()
    );
    
    config.colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
        .pAttachments = blendAttachments.data()
    };
    
    config.dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    
    config.rendering = renderingInfo;
    
    m_pipeline = std::make_unique<Pipeline>(
        m_shared->context,
        m_descriptorLayout->handle(),
        config
    );
}

void GBufferPass::createAttachments() {
    const auto& extent = m_shared->swapChain->extent();
    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
                             VK_IMAGE_USAGE_SAMPLED_BIT | 
                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    
    // Albedo (SRGB)
    m_albedoTexture = m_shared->textureManager->createTexture(
        extent.width, extent.height,
        VK_FORMAT_R8G8B8A8_SRGB,
        usage,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_COLOR_BIT,
        true
    );
    
    // Normal (UNORM)
    m_normalTexture = m_shared->textureManager->createTexture(
        extent.width, extent.height,
        VK_FORMAT_R8G8B8A8_SRGB,
        usage,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_COLOR_BIT,
        true
    );
    
    // Parameters (RG8)
    m_paramTexture = m_shared->textureManager->createTexture(
        extent.width, extent.height,
        VK_FORMAT_R8G8_UNORM,
        usage,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_COLOR_BIT,
        true
    );

    VkImageUsageFlags depthUsage =
           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
           VK_IMAGE_USAGE_SAMPLED_BIT |
           VK_IMAGE_USAGE_TRANSFER_DST_BIT;  // Allow for transitions

    m_depthTexture = m_shared->textureManager->createTexture(
        extent.width, extent.height,
        VK_FORMAT_D32_SFLOAT,
        depthUsage,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        false
    );

    // Update dependencies
    m_dependencies->albedoTexture = &m_albedoTexture;
    m_dependencies->normalTexture = &m_normalTexture;
    m_dependencies->paramTexture = &m_paramTexture;
    m_dependencies->depthTexture = &m_depthTexture;
}

void GBufferPass::createDescriptors() {
    // Descriptor layout (matches original)
    DescriptorSetLayoutBuilder layoutBuilder(m_shared->context->device());
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 
                   static_cast<uint32_t>(m_globalData->modelTextures.size()))
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                   static_cast<uint32_t>(m_globalData->normalTextures.size()))
        .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                   static_cast<uint32_t>(m_globalData->materialTextures.size()))
        .build();
    
    // Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
         static_cast<uint32_t>(m_globalData->modelTextures.size() * MAX_FRAMES_IN_FLIGHT)},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         static_cast<uint32_t>(m_globalData->normalTextures.size() * MAX_FRAMES_IN_FLIGHT)},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         static_cast<uint32_t>(m_globalData->materialTextures.size() * MAX_FRAMES_IN_FLIGHT)}
    };
    
    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_descriptorLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    // Update descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates = {
            {
                .binding = 0,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .bufferInfo = &m_globalData->frameData[i].bufferInfo,
                .descriptorCount = 1,
                .isImage = false
            },
            {
                .binding = 1,
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .imageInfo = m_globalData->frameData[i].textureImageInfos.data(),
                .descriptorCount = static_cast<uint32_t>(m_globalData->modelTextures.size()),
                .isImage = true
            },
            {
                .binding = 2,
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .imageInfo = m_globalData->frameData[i].normalImageInfos.data(),
                .descriptorCount = static_cast<uint32_t>(m_globalData->normalTextures.size()),
                .isImage = true
            },
            {
                .binding = 5,
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .imageInfo = m_globalData->frameData[i].materialImageInfos.data(),
                .descriptorCount = static_cast<uint32_t>(m_globalData->materialTextures.size()),
                .isImage = true
            }
        };
        m_descriptorManager->updateDescriptorSet(i, updates);
    }
}