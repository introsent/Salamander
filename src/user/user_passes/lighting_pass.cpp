#include "../user_passes/lighting_pass.h"

#include "config.h"
#include "pipeline.h"
#include "descriptors/descriptor_set_layout_builder.h"
#include "image_transition_manager.h"

void LightingPass::initialize(const RenderTarget::SharedResources& shared,
                             MainSceneGlobalData& globalData,
                             PassDependencies& dependencies) {
    m_shared = &shared;
    m_globalData = &globalData;
    m_dependencies = &dependencies;
    
    createAttachments();
    createDescriptors();
    createPipeline();
}

void LightingPass::cleanup() {
    m_pipeline.reset();
    m_descriptorManager.reset();
    m_descriptorLayout.reset();
}

void LightingPass::recreateSwapChain() {
    createAttachments();
    updateDescriptors();
}

void LightingPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) {
    // Transition HDR texture
    ImageTransitionManager::transitionColorAttachment(
        cmd, m_hdrTexture.image, 
        VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    
    // Set up HDR attachment
    VkRenderingAttachmentInfo colorAttachment = {
      .sType         = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView     = m_hdrTexture.view,
        .imageLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue    = {0.0f, 0.0f, 0.0f, 1.0f}
    };
    
    VkRenderingInfo renderInfo = {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea           = {{0, 0}, m_shared->swapChain->extent()},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = & colorAttachment,
        .pDepthAttachment     = nullptr
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
    vkCmdBindIndexBuffer(cmd, m_globalData->indexBuffer.handle(), 0, VK_INDEX_TYPE_UINT32);
    // Bind descriptor set
    vkCmdBindDescriptorSets(
        cmd, 
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipeline->layout(),
        0, 1,
        &m_descriptorManager->getDescriptorSets()[frameIndex],
        0, nullptr
    );
    
    // Fullscreen triangle
    vkCmdDraw(cmd, 3, 1, 0, 0);
    
    vkCmdEndRendering(cmd);

    // Transition to shader read
    ImageTransitionManager::transitionToShaderRead(
        cmd,
         m_hdrTexture.image,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void LightingPass::createPipeline() {
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkFormat hdrFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &hdrFormat;
    
    // Blend state (no blending)
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                    VK_COLOR_COMPONENT_G_BIT | 
                                    VK_COLOR_COMPONENT_B_BIT | 
                                    VK_COLOR_COMPONENT_A_BIT;
    
    PipelineConfig config{};
    config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/lighting_vert.spv";
    config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/lighting_frag.spv";
    
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
        .cullMode = VK_CULL_MODE_NONE,
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
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };
    
    config.colorBlendAttachments = {blendAttachment};
    
    config.colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &blendAttachment
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

void LightingPass::createAttachments() {
    const auto& extent = m_shared->swapChain->extent();
    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
                             VK_IMAGE_USAGE_SAMPLED_BIT | 
                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    
    m_hdrTexture = m_shared->textureManager->createTexture(
        extent.width, extent.height,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        usage,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_COLOR_BIT,
        true
    );

    m_dependencies->hdrTexture = &m_hdrTexture;
}

void LightingPass::createDescriptors() {
    // Descriptor layout (matches original)
    DescriptorSetLayoutBuilder layoutBuilder(m_shared->context->device());
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Albedo
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Normal
        .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Params
        .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Depth
        .addBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)          // Lights
        .build();
    
    // Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    };
    
    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_descriptorLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );
    updateDescriptors();
}

void LightingPass::updateDescriptors()
{
    // Update descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo albedoInfo = {
            .sampler = m_globalData->gBufferSampler,
            .imageView = m_dependencies->albedoTexture->view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkDescriptorImageInfo normalInfo = {
            .sampler = m_globalData->gBufferSampler,
            .imageView = m_dependencies->normalTexture->view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkDescriptorImageInfo paramsInfo = {
            .sampler = m_globalData->gBufferSampler,
            .imageView = m_dependencies->paramTexture->view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkDescriptorImageInfo depthInfo = {
            .sampler = m_globalData->depthSampler,
            .imageView = m_dependencies->depthTexture->view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        };

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
            },
            {
                .binding = 4,
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .imageInfo = &depthInfo,
                .descriptorCount = 1,
                .isImage = true
            },
            {
                .binding = 5,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .bufferInfo = &m_globalData->frameData[i].omniLightBufferInfo,
                .descriptorCount = 1,
                .isImage = false
            }
        };
        m_descriptorManager->updateDescriptorSet(i, updates);
    }
}
