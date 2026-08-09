#include "gbuffer_pass.h"
#include "config.h"
#include "pass_dependencies.h"
#include "scene_data.h"
#include "graphics/descriptors/descriptor_set_layout_builder.h"
#include "pipeline/push_constants.h"
#include "textures/texture_manager.h"

namespace Salamander::Renderer::Passes {

    void GBufferPass::initialize(const Frame::RenderContext &ctx,
                                 Scene::MainSceneData &globalData,
                                 PassDependencies &dependencies) {
        m_ctx = &ctx;
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
    }

    void GBufferPass::recreateSwapChain() {
        // clean up old attachments by key
        m_ctx->textureManager().destroyTexture("Albedo");
        m_ctx->textureManager().destroyTexture("Normal");
        m_ctx->textureManager().destroyTexture("Param");

        // reset pointers
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            m_albedoTextures[i] = nullptr;
            m_normalTextures[i] = nullptr;
            m_paramTextures[i] = nullptr;
        }
        // create new attachments with new extent
        createAttachments();

        // update descriptors
        updateDescriptors();
    }

    void GBufferPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t /*imageIndex*/) {
        const std::array<VkRenderingAttachmentInfo, 3> colorAttachments = {{
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = m_albedoTextures[frameIndex]->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {.color = {0.f, 0.f, 0.f, 1.f}}
            },
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = m_normalTextures[frameIndex]->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {.color = {0.f, 0.f, 0.f, 0.f}}
            },
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = m_paramTextures[frameIndex]->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {.color = {0.f, 0.f, 0.f, 0.f}}
            }
        }};

        const VkRenderingAttachmentInfo depthAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_ctx->frames()[frameIndex].depthTexture->getDescriptorInfo().imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE
        };

        const VkExtent2D extent = m_ctx->swapChain().extent();
        const VkRenderingInfo renderInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, extent},
            .layerCount = 1,
            .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
            .pColorAttachments = colorAttachments.data(),
            .pDepthAttachment = &depthAttachment
        };

        vkCmdBeginRendering(cmd, &renderInfo);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());

        const VkViewport viewport{0.f, 0.f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.f, 1.f};
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        const VkRect2D scissor{{0, 0}, extent};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipeline->layout(), 0, 1,
            &m_descriptorManager->getDescriptorSets()[frameIndex], 0, nullptr);

        vkCmdBindIndexBuffer(cmd, m_globalData->indexBuffer.handle(), 0, VK_INDEX_TYPE_UINT32);

        for (const auto &primitive : m_globalData->primitives) {
            const Graphics::Pipeline::PushConstants pc{
                .vertexBufferAddress = m_globalData->vertexBufferAddress,
                .baseColorTextureIndex = primitive.materialIndex,
                .metalRoughTextureIndex = primitive.metalRoughTextureIndex,
                .normalTextureIndex = primitive.normalTextureIndex,
                .textureCount = static_cast<uint32_t>(m_globalData->modelTextures.size()),
                .modelScale = m_globalData->modelScale
            };
            vkCmdPushConstants(cmd, m_pipeline->layout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                               sizeof(Graphics::Pipeline::PushConstants), &pc);
            vkCmdDrawIndexed(cmd, primitive.indexCount, 1, primitive.indexOffset, 0, 0);
        }

        vkCmdEndRendering(cmd);
    }

    void GBufferPass::createAttachments() {
        const auto extent = m_ctx->swapChain().extent();
        constexpr VkImageUsageFlags usage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        auto &tm = m_ctx->textureManager();
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            const std::string idx = std::to_string(i);
            m_albedoTextures[i] = &tm.createTexture(extent.width, extent.height, VK_FORMAT_R8G8B8A8_SRGB,
                usage, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_ASPECT_COLOR_BIT, false, true,
                "Albedo");
            m_normalTextures[i] = &tm.createTexture(extent.width, extent.height, VK_FORMAT_R8G8B8A8_SRGB,
                usage, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_ASPECT_COLOR_BIT, false, true,
                "Normal");
            m_paramTextures[i] = &tm.createTexture(extent.width, extent.height, VK_FORMAT_R8G8_UNORM,
                usage, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_ASPECT_COLOR_BIT, false, true,
                "Param");
            m_dependencies->albedoTextures[i] = m_albedoTextures[i];
            m_dependencies->normalTextures[i] = m_normalTextures[i];
            m_dependencies->paramTextures[i] = m_paramTextures[i];
        }
    }

    void GBufferPass::createDescriptors() {
        const auto albedoCount = static_cast<uint32_t>(m_globalData->modelTextures.size());
        const auto normalCount = static_cast<uint32_t>(m_globalData->normalTextures.size());
        const auto materialCount = static_cast<uint32_t>(m_globalData->materialTextures.size());

        Graphics::Descriptors::DescriptorSetLayoutBuilder layoutBuilder(m_ctx->context().device());
        m_descriptorLayout = layoutBuilder
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, albedoCount)
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, normalCount)
            .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, materialCount)
            .build();

        const std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, albedoCount * MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, normalCount * MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, materialCount * MAX_FRAMES_IN_FLIGHT}
        };

        m_descriptorManager = std::make_unique<Graphics::Descriptors::MainDescriptorManager>(
            m_ctx->context().device(), m_descriptorLayout->handle(), poolSizes, MAX_FRAMES_IN_FLIGHT
        );

        updateDescriptors();
    }

    void GBufferPass::updateDescriptors() const {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            const auto &fd = m_globalData->frameData[i];

            Graphics::Descriptors::MainDescriptorManager::DescriptorUpdateInfo ubo{};
            ubo.binding = 0;
            ubo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ubo.bufferInfo = &fd.bufferInfo;
            ubo.descriptorCount = 1;
            ubo.isImage = false;

            Graphics::Descriptors::MainDescriptorManager::DescriptorUpdateInfo albedo{};
            albedo.binding = 1;
            albedo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            albedo.imageInfo = fd.textureImageInfos.data();
            albedo.descriptorCount = static_cast<uint32_t>(m_globalData->modelTextures.size());
            albedo.isImage = true;

            Graphics::Descriptors::MainDescriptorManager::DescriptorUpdateInfo normal{};
            normal.binding = 2;
            normal.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            normal.imageInfo = fd.normalImageInfos.data();
            normal.descriptorCount = static_cast<uint32_t>(m_globalData->normalTextures.size());
            normal.isImage = true;

            Graphics::Descriptors::MainDescriptorManager::DescriptorUpdateInfo material{};
            material.binding = 5;
            material.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            material.imageInfo = fd.materialImageInfos.data();
            material.descriptorCount = static_cast<uint32_t>(m_globalData->materialTextures.size());
            material.isImage = true;

            m_descriptorManager->updateDescriptorSet(i, {ubo, albedo, normal, material});
        }
    }

    void GBufferPass::createPipeline() {
        static constexpr std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };
        constexpr std::array<VkFormat, 3> colorFormats = {
            VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8_UNORM
        };
        const VkPipelineRenderingCreateInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = static_cast<uint32_t>(colorFormats.size()),
            .pColorAttachmentFormats = colorFormats.data(),
            .depthAttachmentFormat = m_ctx->depthFormat()
        };
        std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachments{};
        for (auto &a : blendAttachments) {
            a.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        }
        Graphics::Pipeline::PipelineConfig config{};
        config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/gbuffer_vert.spv";
        config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/gbuffer_frag.spv";
        config.inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                   .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        config.viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                   .viewportCount = 1, .scissorCount = 1};
        config.rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                             .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_BACK_BIT,
                             .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, .lineWidth = 1.0f};
        config.multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
        config.depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                               .depthTestEnable = VK_TRUE, .depthWriteEnable = VK_FALSE,
                               .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL};
        config.colorBlendAttachments = std::vector<VkPipelineColorBlendAttachmentState>(
            blendAttachments.begin(), blendAttachments.end());
        config.colorBlending = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
                                .pAttachments = blendAttachments.data()};
        config.dynamicState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                               .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                               .pDynamicStates = dynamicStates.data()};
        config.rendering = renderingInfo;

        m_pipeline = std::make_unique<Graphics::Pipeline::Pipeline>(
            &m_ctx->context(), m_descriptorLayout->handle(), config
        );
    }
}
