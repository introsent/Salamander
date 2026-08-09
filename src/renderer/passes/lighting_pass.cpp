#include "lighting_pass.h"
#include "config.h"
#include "pass_dependencies.h"
#include "graphics/descriptors/descriptor_set_layout_builder.h"
#include "textures/texture_manager.h"

namespace Salamander::Renderer::Passes {
    void LightingPass::initialize(const Frame::RenderContext &ctx,
                                  Scene::MainSceneData &globalData,
                                  PassDependencies &dependencies) {
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
        m_ctx->textureManager().destroyTexture("HDR");

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            m_hdrTextures[i] = nullptr;
        }

        createAttachments();
        updateDescriptors();
    }

    void LightingPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t /*imageIndex*/) {
        const VkRenderingAttachmentInfo colorAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_hdrTextures[frameIndex]->getDescriptorInfo().imageView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
        };

        const VkExtent2D extent = m_ctx->swapChain().extent();
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

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipeline->layout(), 0, 1,
            &m_descriptorManager->getDescriptorSets()[frameIndex], 0, nullptr);

        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

    void LightingPass::createAttachments() {
        const auto extent = m_ctx->swapChain().extent();
        constexpr VkImageUsageFlags usage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        auto &tm = m_ctx->textureManager();
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            m_hdrTextures[i] = &tm.createTexture(extent.width, extent.height, VK_FORMAT_R32G32B32A32_SFLOAT,
                usage, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_ASPECT_COLOR_BIT, false, true, "HDR");
            m_dependencies->hdrTextures[i] = m_hdrTextures[i];
        }
    }

    void LightingPass::createDescriptors() {
        Graphics::Descriptors::DescriptorSetLayoutBuilder layoutBuilder(m_ctx->context().device());
        m_descriptorLayout = layoutBuilder
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Albedo
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Normal
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Params
            .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Depth
            .addBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)         // Lights
            .addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Cube map
            .addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Irradiance
            .addBinding(8, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)         // Dir light
            .addBinding(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Shadow map
            .build();

        const std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 * MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6 * MAX_FRAMES_IN_FLIGHT}
        };

        m_descriptorManager = std::make_unique<Graphics::Descriptors::MainDescriptorManager>(
            m_ctx->context().device(), m_descriptorLayout->handle(), poolSizes, MAX_FRAMES_IN_FLIGHT
        );

        updateDescriptors();
    }

    void LightingPass::updateDescriptors() const {
        using UpdateInfo = Graphics::Descriptors::MainDescriptorManager::DescriptorUpdateInfo;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            const auto &fd = m_globalData->frameData[i];
            const auto &depthTex = m_ctx->frames()[i].depthTexture;

            VkDescriptorImageInfo albedoInfo{m_dependencies->albedoTextures[i]->getDescriptorInfo().sampler,
                m_dependencies->albedoTextures[i]->getDescriptorInfo().imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkDescriptorImageInfo normalInfo{m_dependencies->normalTextures[i]->getDescriptorInfo().sampler,
                m_dependencies->normalTextures[i]->getDescriptorInfo().imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkDescriptorImageInfo paramsInfo{m_dependencies->paramTextures[i]->getDescriptorInfo().sampler,
                m_dependencies->paramTextures[i]->getDescriptorInfo().imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkDescriptorImageInfo depthInfo{depthTex->getDescriptorInfo().sampler,
                depthTex->getDescriptorInfo().imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
            VkDescriptorImageInfo cubeInfo{m_dependencies->cubeMap->getDescriptorInfo().sampler,
                m_dependencies->cubeMap->getDescriptorInfo().imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkDescriptorImageInfo irradianceInfo{m_dependencies->irradianceMap->getDescriptorInfo().sampler,
                m_dependencies->irradianceMap->getDescriptorInfo().imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkDescriptorImageInfo shadowInfo{m_dependencies->shadowMap->getDescriptorInfo().sampler,
                m_dependencies->shadowMap->getDescriptorInfo().imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

            UpdateInfo ubo{}; ubo.binding = 0; ubo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ubo.bufferInfo = &fd.bufferInfo; ubo.descriptorCount = 1; ubo.isImage = false;

            UpdateInfo albedo{}; albedo.binding = 1; albedo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            albedo.imageInfo = &albedoInfo; albedo.descriptorCount = 1; albedo.isImage = true;

            UpdateInfo normal{}; normal.binding = 2; normal.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            normal.imageInfo = &normalInfo; normal.descriptorCount = 1; normal.isImage = true;

            UpdateInfo params{}; params.binding = 3; params.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            params.imageInfo = &paramsInfo; params.descriptorCount = 1; params.isImage = true;

            UpdateInfo depth{}; depth.binding = 4; depth.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            depth.imageInfo = &depthInfo; depth.descriptorCount = 1; depth.isImage = true;

            UpdateInfo lights{}; lights.binding = 5; lights.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            lights.bufferInfo = &fd.omniLightBufferInfo; lights.descriptorCount = 1; lights.isImage = false;

            UpdateInfo cube{}; cube.binding = 6; cube.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            cube.imageInfo = &cubeInfo; cube.descriptorCount = 1; cube.isImage = true;

            UpdateInfo irradiance{}; irradiance.binding = 7; irradiance.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            irradiance.imageInfo = &irradianceInfo; irradiance.descriptorCount = 1; irradiance.isImage = true;

            UpdateInfo dirLight{}; dirLight.binding = 8; dirLight.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            dirLight.bufferInfo = &fd.directionalLightBufferInfo; dirLight.descriptorCount = 1; dirLight.isImage = false;

            UpdateInfo shadow{}; shadow.binding = 9; shadow.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            shadow.imageInfo = &shadowInfo; shadow.descriptorCount = 1; shadow.isImage = true;

            m_descriptorManager->updateDescriptorSet(i, {ubo, albedo, normal, params, depth, lights, cube, irradiance, dirLight, shadow});
        }
    }

    void LightingPass::createPipeline() {
        static constexpr std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };
        VkFormat hdrFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        const VkPipelineRenderingCreateInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &hdrFormat
        };
        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        Graphics::Pipeline::PipelineConfig config{};
        config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/lighting_vert.spv";
        config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/lighting_frag.spv";
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
            &m_ctx->context(), m_descriptorLayout->handle(), config
        );
    }
}
