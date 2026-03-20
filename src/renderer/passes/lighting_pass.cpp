#include "lighting_pass.h"

#include "config.h"
#include "descriptors/descriptor_set_layout_builder.h"

namespace Salamander::Renderer::Passes {
    void LightingPass::initialize(const Salamander::Renderer::RenderContext &ctx,
                                       Salamander::Scene::MainSceneData &globalData,
                                       Salamander::Renderer::PassDependencies &dependencies) {
        m_ctx = &ctx;
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
        for (int i = 0; i < Salamander::Renderer::MAX_FRAMES_IN_FLIGHT; ++i)
            m_ctx->textureManager().destroyTexture("HDR_" + std::to_string(i));

        for (int i = 0; i < Salamander::Renderer::MAX_FRAMES_IN_FLIGHT; ++i)
            m_hdrTextures[i] = nullptr;

        createAttachments();
        updateDescriptors();
    }

    void LightingPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t /*imageIndex*/) {
        m_hdrTextures[frameIndex]->getImage()->transitionLayoutEx(
            cmd,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
        );

        VkRenderingAttachmentInfo colorAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_hdrTextures[frameIndex]->getDescriptorInfo().imageView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
        };

        const VkExtent2D extent = m_ctx->swapChain().extent();

        VkRenderingInfo renderInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment
        };

        vkCmdBeginRendering(cmd, &renderInfo);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());

        VkViewport viewport{
            0.0f, 0.0f,
            static_cast<float>(extent.width), static_cast<float>(extent.height),
            0.0f, 1.0f
        };
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{{0, 0}, extent};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindIndexBuffer(cmd, m_globalData->indexBuffer.handle(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->layout(),
                                0, 1, &m_descriptorManager->getDescriptorSets()[frameIndex], 0, nullptr);

        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);

        m_hdrTextures[frameIndex]->getImage()->transitionLayoutEx(
            cmd,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_SHADER_WRITE_BIT
        );
    }

    void LightingPass::createPipeline() {
        static constexpr std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };

        VkFormat hdrFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        VkPipelineRenderingCreateInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &hdrFormat
        };

        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        PipelineConfig config{};
        config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/lighting_vert.spv";
        config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/lighting_frag.spv";
        config.inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        };
        config.viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1, .scissorCount = 1
        };
        config.rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth = 1.0f
        };
        config.multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        };
        config.depthStencil = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE
        };
        config.colorBlendAttachments = {blendAttachment};
        config.colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &blendAttachment
        };
        config.dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };
        config.rendering = renderingInfo;

        m_pipeline = std::make_unique<Pipeline>(&m_ctx->context(), m_descriptorLayout->handle(), config);
    }

    void LightingPass::createAttachments() {
        const auto extent = m_ctx->swapChain().extent();
        constexpr VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                            VK_IMAGE_USAGE_SAMPLED_BIT |
                                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                            VK_IMAGE_USAGE_STORAGE_BIT;

        for (int i = 0; i < Salamander::Renderer::MAX_FRAMES_IN_FLIGHT; ++i) {
            m_hdrTextures[i] = &m_ctx->textureManager().createTexture(
                extent.width, extent.height, VK_FORMAT_R32G32B32A32_SFLOAT,
                usage, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_ASPECT_COLOR_BIT,
                false, true, "HDR_" + std::to_string(i)
            );
            m_dependencies->hdrTextures[i] = m_hdrTextures[i];
        }
    }

    void LightingPass::createDescriptors() {
        DescriptorSetLayoutBuilder layoutBuilder(m_ctx->context().device());
        m_descriptorLayout = layoutBuilder
                .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Albedo
                .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Normal
                .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Params
                .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Depth
                .addBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Lights
                .addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Cube map
                .addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                // Irradiance map
                .addBinding(8, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Directional light
                .addBinding(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Shadow map
                .build();

        std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 * Salamander::Renderer::MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6 * Salamander::Renderer::MAX_FRAMES_IN_FLIGHT}
        };

        m_descriptorManager = std::make_unique<MainDescriptorManager>(
            m_ctx->context().device(), m_descriptorLayout->handle(), poolSizes,
            Salamander::Renderer::MAX_FRAMES_IN_FLIGHT
        );

        updateDescriptors();
    }

    void LightingPass::updateDescriptors() const {
        for (size_t i = 0; i < Salamander::Renderer::MAX_FRAMES_IN_FLIGHT; ++i) {
            const auto &depthTex = m_ctx->frames()[i].depthTexture;

            VkDescriptorImageInfo albedoInfo{
                .sampler = m_dependencies->albedoTextures[i]->getDescriptorInfo().sampler,
                .imageView = m_dependencies->albedoTextures[i]->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            VkDescriptorImageInfo normalInfo{
                .sampler = m_dependencies->normalTextures[i]->getDescriptorInfo().sampler,
                .imageView = m_dependencies->normalTextures[i]->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            VkDescriptorImageInfo paramsInfo{
                .sampler = m_dependencies->paramTextures[i]->getDescriptorInfo().sampler,
                .imageView = m_dependencies->paramTextures[i]->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            VkDescriptorImageInfo depthInfo{
                .sampler = depthTex->getDescriptorInfo().sampler,
                .imageView = depthTex->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            };
            VkDescriptorImageInfo cubeInfo{
                .sampler = m_dependencies->cubeMap->getDescriptorInfo().sampler,
                .imageView = m_dependencies->cubeMap->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            VkDescriptorImageInfo irradianceInfo{
                .sampler = m_dependencies->irradianceMap->getDescriptorInfo().sampler,
                .imageView = m_dependencies->irradianceMap->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            VkDescriptorImageInfo shadowInfo{
                .sampler = m_dependencies->shadowMap->getDescriptorInfo().sampler,
                .imageView = m_dependencies->shadowMap->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            };

            std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates = {
                {
                    .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .bufferInfo = &m_globalData->frameData[i].bufferInfo, .descriptorCount = 1, .isImage = false
                },
                {
                    .binding = 1, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .imageInfo = &albedoInfo, .descriptorCount = 1, .isImage = true
                },
                {
                    .binding = 2, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .imageInfo = &normalInfo, .descriptorCount = 1, .isImage = true
                },
                {
                    .binding = 3, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .imageInfo = &paramsInfo, .descriptorCount = 1, .isImage = true
                },
                {
                    .binding = 4, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .imageInfo = &depthInfo, .descriptorCount = 1, .isImage = true
                },
                {
                    .binding = 5, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .bufferInfo = &m_globalData->frameData[i].omniLightBufferInfo, .descriptorCount = 1,
                    .isImage = false
                },
                {
                    .binding = 6, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .imageInfo = &cubeInfo, .descriptorCount = 1, .isImage = true
                },
                {
                    .binding = 7, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .imageInfo = &irradianceInfo, .descriptorCount = 1, .isImage = true
                },
                {
                    .binding = 8, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .bufferInfo = &m_globalData->frameData[i].directionalLightBufferInfo, .descriptorCount = 1,
                    .isImage = false
                },
                {
                    .binding = 9, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .imageInfo = &shadowInfo, .descriptorCount = 1, .isImage = true
                }
            };

            m_descriptorManager->updateDescriptorSet(i, updates);
        }
    }
}
