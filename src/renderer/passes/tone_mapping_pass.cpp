#include "tone_mapping_pass.h"
#include "config.h"
#include "pass_dependencies.h"
#include "graphics/descriptors/descriptor_set_layout_builder.h"
#include "textures/texture.h"

namespace Salamander::Renderer::Passes {

    void ToneMappingPass::initialize(const Frame::RenderContext &ctx,
                                     Scene::MainSceneData &globalData,
                                     PassDependencies &dependencies) {
        m_ctx = &ctx;
        m_globalData = &globalData;
        m_dependencies = &dependencies;
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
        const VkExtent2D extent = m_ctx->swapChain().extent();

        const VkRenderingAttachmentInfo colorAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_ctx->swapChain().imagesViews()[imageIndex],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {0.0f, 0.0f, 0.0f, 1.0f}
        };
        const VkRenderingInfo renderInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment
        };

        vkCmdBeginRendering(cmd, &renderInfo);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());

        const VkViewport viewport{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        const VkRect2D scissor{{0, 0}, extent};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->layout(),
            0, 1, &m_descriptorManager->getDescriptorSets()[frameIndex], 0, nullptr);

        const TonePush pc{.resolution = {static_cast<float>(extent.width), static_cast<float>(extent.height)}};
        vkCmdPushConstants(cmd, m_pipeline->layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

    void ToneMappingPass::createDescriptors() {
        Graphics::Descriptors::DescriptorSetLayoutBuilder layoutBuilder(m_ctx->context().device());
        m_descriptorLayout = layoutBuilder
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        const std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT}
        };
        m_descriptorManager = std::make_unique<Graphics::Descriptors::MainDescriptorManager>(
            m_ctx->context().device(), m_descriptorLayout->handle(), poolSizes, MAX_FRAMES_IN_FLIGHT
        );

        updateDescriptors();
    }

    void ToneMappingPass::updateDescriptors() const {
        using UpdateInfo = Graphics::Descriptors::MainDescriptorManager::DescriptorUpdateInfo;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VkDescriptorImageInfo hdrInfo{
                .sampler = m_dependencies->hdrTextures[i]->getDescriptorInfo().sampler,
                .imageView = m_dependencies->hdrTextures[i]->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            VkDescriptorImageInfo luminanceInfo{
                .sampler = m_dependencies->averageLuminanceTextures[i]->getDescriptorInfo().sampler,
                .imageView = m_dependencies->averageLuminanceTextures[i]->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            };

            UpdateInfo hdr{}; hdr.binding = 0; hdr.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            hdr.imageInfo = &hdrInfo; hdr.descriptorCount = 1; hdr.isImage = true;

            UpdateInfo exposure{}; exposure.binding = 1; exposure.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            exposure.bufferInfo = &m_globalData->frameData[i].cameraExposureBufferInfo;
            exposure.descriptorCount = 1; exposure.isImage = false;

            UpdateInfo luminance{}; luminance.binding = 2; luminance.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            luminance.imageInfo = &luminanceInfo; luminance.descriptorCount = 1; luminance.isImage = true;

            m_descriptorManager->updateDescriptorSet(i, {hdr, exposure, luminance});
        }
    }

    void ToneMappingPass::createPipeline() {
        static constexpr std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };
        VkFormat swapFormat = m_ctx->swapChain().format();
        const VkPipelineRenderingCreateInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapFormat
        };
        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.blendEnable = VK_FALSE;
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        const VkPushConstantRange pushRange{
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(TonePush)
        };
        Graphics::Pipeline::PipelineConfig config{};
        config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/tone_vert.spv";
        config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/tone_frag.spv";
        config.inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        config.viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                .viewportCount = 1, .scissorCount = 1};
        config.rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                             .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE,
                             .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, .lineWidth = 1.0f};
        config.multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
        config.depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                               .depthTestEnable = VK_FALSE, .depthWriteEnable = VK_FALSE};
        config.colorBlendAttachments = {blendAttachment};
        config.colorBlending = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                .attachmentCount = 1, .pAttachments = &blendAttachment};
        config.dynamicState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                               .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                               .pDynamicStates = dynamicStates.data()};
        config.rendering = renderingInfo;

        m_pipeline = std::make_unique<Graphics::Pipeline::Pipeline>(
            &m_ctx->context(), m_descriptorLayout->handle(), config, pushRange
        );
    }
}
