#include "../user_passes/tone_mapping_pass.h"
#include "config.h"
#include "image_transition_manager.h"
#include "pipeline.h"
#include "descriptors/descriptor_set_layout_builder.h"

void ToneMappingPass::initialize(const RenderTarget::SharedResources& shared,
                                 MainSceneGlobalData& globalData,
                                 PassDependencies& dependencies) {
    m_shared = &shared;
    m_globalData = &globalData;
    m_dependencies = &dependencies;

    size_t count = m_shared->swapChain->imagesViews().size();
    m_currentColorLayouts.assign(count, VK_IMAGE_LAYOUT_UNDEFINED);

    createDescriptors();
    createPipeline();
}

void ToneMappingPass::cleanup() {
    m_pipeline.reset();
    m_descriptorManager.reset();
    m_descriptorLayout.reset();
}

void ToneMappingPass::recreateSwapChain() {
    updateDescriptors();
}

void ToneMappingPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) {
    VkImage swapImage = m_shared->swapChain->getCurrentImage(frameIndex);

    // ─── Set up your color attachment for tone mapping ───
    VkRenderingAttachmentInfo colorAttachment = {
        .sType         = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView     = m_shared->swapChain->imagesViews()[imageIndex],
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
        .pColorAttachments    = &colorAttachment,
        .pDepthAttachment     = nullptr
    };

    vkCmdBeginRendering(cmd, &renderInfo);
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());

      // dynamic viewport/scissor
      VkViewport viewport = {
          0.0f, 0.0f,
          float(m_shared->swapChain->extent().width),
          float(m_shared->swapChain->extent().height),
          0.0f, 1.0f
      };
      vkCmdSetViewport(cmd, 0, 1, &viewport);
      VkRect2D scissor = {{0, 0}, m_shared->swapChain->extent()};
      vkCmdSetScissor(cmd, 0, 1, &scissor);

      // bind your tone‐mapping descriptor set (same as before)
      vkCmdBindDescriptorSets(
          cmd,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          m_pipeline->layout(),
          0, 1,
          &m_descriptorManager->getDescriptorSets()[frameIndex],
          0, nullptr
      );

      // push constants
      TonePush pc {
          glm::vec2{
              float(m_shared->swapChain->extent().width),
              float(m_shared->swapChain->extent().height)
          }
      };
      vkCmdPushConstants(
          cmd,
          m_pipeline->layout(),
          VK_SHADER_STAGE_FRAGMENT_BIT,
          0, sizeof(TonePush),
          &pc
      );

      // fullscreen triangle
      vkCmdDraw(cmd, 3, 1, 0, 0);
    vkCmdEndRendering(cmd);
}

void ToneMappingPass::createPipeline() {
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkFormat swapFormat = m_shared->swapChain->format();
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &swapFormat;
    renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED; // Explicit for clarity

    // Blend state (no blending)
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.blendEnable = VK_FALSE; // Explicitly disabled
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                   VK_COLOR_COMPONENT_G_BIT |
                                   VK_COLOR_COMPONENT_B_BIT |
                                   VK_COLOR_COMPONENT_A_BIT;

    // Push constant range
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(TonePush);

    PipelineConfig config{};
    config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/tone_vert.spv";
    config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/tone_frag.spv";

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
        .depthCompareOp = VK_COMPARE_OP_ALWAYS, // Explicit for completeness
        .stencilTestEnable = VK_FALSE           // Explicit for completeness
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
        config,
        pushRange
    );
}

void ToneMappingPass::createDescriptors() {
    // Descriptor layout
    DescriptorSetLayoutBuilder layoutBuilder(m_shared->context->device());
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();
    
    // Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT}
    };
    
    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_descriptorLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );
    
    updateDescriptors();
}

void ToneMappingPass::updateDescriptors()
{
    // Update descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo hdrInfo = {
            .sampler = m_globalData->hdrSampler,
            .imageView = m_dependencies->hdrTexture->view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates = {
            {
                .binding = 0,
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .imageInfo = &hdrInfo,
                .descriptorCount = 1,
                .isImage = true
            },
            {
                .binding = 1,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .bufferInfo = &m_globalData->frameData[i].cameraExposureBufferInfo,
                .descriptorCount = 1,
                .isImage = false
            }
        };
        m_descriptorManager->updateDescriptorSet(i, updates);
    }
}
