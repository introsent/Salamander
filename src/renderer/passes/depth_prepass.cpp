#include "depth_prepass.h"

#include "config.h"
#include "depth_format.h"
#include "scene_data.h"
#include "descriptors/descriptor_set_layout_builder.h"
#include "pipeline/push_constants.h"

#ifdef USE_TINYGLTF
    #include "loaders/tinygltf_loader.h"
#endif

namespace Salamander::Renderer::Passes {
    void DepthPrepass::initialize(const RenderContext &ctx,
                                       Salamander::Scene::MainSceneData &globalData,
                                       PassDependencies &dependencies) {
        m_ctx = &ctx;
        m_globalData = &globalData;
        m_dependencies = &dependencies;

        createDescriptors();
        createPipeline();
    }

    void DepthPrepass::cleanup() {
        m_pipeline.reset();
        m_descriptorManager.reset();
        m_descriptorLayout.reset();
    }

    void DepthPrepass::recreateSwapChain() {
    }

    void DepthPrepass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t /*imageIndex*/) {
        auto *depthImage = m_ctx->frames()[frameIndex].depthTexture->getImage();

        depthImage->transitionLayoutEx(
            cmd,
            depthImage->currentLayout(),
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        );

        VkRenderingAttachmentInfo depthAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_ctx->frames()[frameIndex].depthTexture->getDescriptorInfo().imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.depthStencil = {1.0f, 0}}
        };

        const VkExtent2D extent = m_ctx->swapChain().extent();

        VkRenderingInfo renderInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, extent},
            .layerCount = 1,
            .colorAttachmentCount = 0,
            .pDepthAttachment = &depthAttachment
        };

        vkCmdBeginRendering(cmd, &renderInfo);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());

        VkViewport viewport{
            0.0f, 0.0f,
            static_cast<float>(extent.width),
            static_cast<float>(extent.height),
            0.0f, 1.0f
        };
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{{0, 0}, extent};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipeline->layout(),
            0, 1,
            &m_descriptorManager->getDescriptorSets()[frameIndex],
            0, nullptr
        );

        vkCmdBindIndexBuffer(cmd, m_globalData->indexBuffer.handle(), 0, VK_INDEX_TYPE_UINT32);

        for (const auto &primitive: m_globalData->primitives) {
            Salamander::Graphics::PushConstants pc{
                .vertexBufferAddress = m_globalData->vertexBufferAddress,
                .baseColorTextureIndex = primitive.materialIndex,
                .metalRoughTextureIndex = primitive.metalRoughTextureIndex,
                .normalTextureIndex = primitive.normalTextureIndex,
                .textureCount = static_cast<uint32_t>(m_globalData->modelTextures.size()),
                .modelScale = m_globalData->modelScale
            };
            vkCmdPushConstants(cmd, m_pipeline->layout(),
                               VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Salamander::Graphics::PushConstants), &pc);
            vkCmdDrawIndexed(cmd, primitive.indexCount, 1, primitive.indexOffset, 0, 0);
        }

        vkCmdEndRendering(cmd);

        depthImage->transitionLayoutEx(
            cmd,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_2_SHADER_READ_BIT
        );
    }

    void DepthPrepass::createPipeline() {
        static constexpr std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineRenderingCreateInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .depthAttachmentFormat = m_ctx->depthFormat()
        };

        PipelineConfig config{};
        config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/depth_vert.spv";
        config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/depth_frag.spv";

        config.inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        };
        config.viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1
        };
        config.rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth = 1.0f
        };
        config.multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        };
        config.depthStencil = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS
        };
        config.colorBlendAttachments.clear();
        config.colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 0
        };
        config.dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };
        config.rendering = renderingInfo;

        m_pipeline = std::make_unique<Pipeline>(
            &m_ctx->context(),
            m_descriptorLayout->handle(),
            config
        );
    }

    void DepthPrepass::createDescriptors() {
        const uint32_t textureCount = static_cast<uint32_t>(m_globalData->modelTextures.size());

        DescriptorSetLayoutBuilder layoutBuilder(m_ctx->context().device());
        m_descriptorLayout = layoutBuilder
                .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, textureCount)
                .build();

        std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textureCount * MAX_FRAMES_IN_FLIGHT}
        };

        m_descriptorManager = std::make_unique<MainDescriptorManager>(
            m_ctx->context().device(),
            m_descriptorLayout->handle(),
            poolSizes,
            MAX_FRAMES_IN_FLIGHT
        );

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
                    .descriptorCount = textureCount,
                    .isImage = true
                }
            };
            m_descriptorManager->updateDescriptorSet(i, updates);
        }
    }
}
