#include "shadow_pass.h"
#include "config.h"
#include "pass_dependencies.h"
#include "graphics/descriptors/descriptor_set_layout_builder.h"
#include "textures/texture.h"

namespace Salamander::Renderer::Passes {

    void ShadowPass::initialize(const Frame::RenderContext &ctx,
                                Scene::MainSceneData &globalData,
                                PassDependencies &dependencies) {
        m_ctx = &ctx;
        m_globalData = &globalData;
        m_dependencies = &dependencies;
        createLightMatrices();
        createUniformBuffers();
        createDescriptors();
        createPipeline();
    }

    void ShadowPass::cleanup() {}
    void ShadowPass::recreateSwapChain() {}

    void ShadowPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t /*imageIndex*/) {
        auto *shadowImage = m_dependencies->shadowMap->getImage();
        const glm::ivec2 shadowSize = shadowImage->size();

        shadowImage->transitionLayoutEx(
            cmd,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        );

        const VkRenderingAttachmentInfo depthAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_dependencies->shadowMap->getDescriptorInfo().imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.depthStencil = {1.0f, 0}}
        };

        const VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, {static_cast<uint32_t>(shadowSize.x), static_cast<uint32_t>(shadowSize.y)}},
            .layerCount = 1,
            .pDepthAttachment = &depthAttachment
        };

        vkCmdBeginRendering(cmd, &renderingInfo);

        const VkViewport viewport{0.0f, 0.0f, static_cast<float>(shadowSize.x), static_cast<float>(shadowSize.y), 0.0f, 1.0f};
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        const VkRect2D scissor{{0, 0}, {static_cast<uint32_t>(shadowSize.x), static_cast<uint32_t>(shadowSize.y)}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->layout(),
            0, 1, &m_descriptorManager->getDescriptorSets()[frameIndex], 0, nullptr);
        vkCmdBindIndexBuffer(cmd, m_globalData->indexBuffer.handle(), 0, VK_INDEX_TYPE_UINT32);

        for (const auto &primitive : m_globalData->primitives) {
            const ShadowPushConstants pc{
                .vertexBufferAddress = m_globalData->vertexBufferAddress,
                .modelScale = globalScale,
                .baseColorTextureIndex = primitive.materialIndex
            };
            vkCmdPushConstants(cmd, m_pipeline->layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
            vkCmdDrawIndexed(cmd, primitive.indexCount, 1, primitive.indexOffset, 0, 0);
        }

        vkCmdEndRendering(cmd);

        shadowImage->transitionLayoutEx(
            cmd,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
        );
    }

    void ShadowPass::createLightMatrices() const {
        const glm::vec3 sceneCenter = (m_globalData->sceneAABB.min + m_globalData->sceneAABB.max) / 2.0f;
        const glm::vec3 lightDirection = directionalLight.directionalLightDirection;

        const std::vector<glm::vec3> corners = {
            {m_globalData->sceneAABB.min.x, m_globalData->sceneAABB.min.y, m_globalData->sceneAABB.min.z},
            {m_globalData->sceneAABB.max.x, m_globalData->sceneAABB.min.y, m_globalData->sceneAABB.min.z},
            {m_globalData->sceneAABB.min.x, m_globalData->sceneAABB.max.y, m_globalData->sceneAABB.min.z},
            {m_globalData->sceneAABB.max.x, m_globalData->sceneAABB.max.y, m_globalData->sceneAABB.min.z},
            {m_globalData->sceneAABB.min.x, m_globalData->sceneAABB.min.y, m_globalData->sceneAABB.max.z},
            {m_globalData->sceneAABB.max.x, m_globalData->sceneAABB.min.y, m_globalData->sceneAABB.max.z},
            {m_globalData->sceneAABB.min.x, m_globalData->sceneAABB.max.y, m_globalData->sceneAABB.max.z},
            {m_globalData->sceneAABB.max.x, m_globalData->sceneAABB.max.y, m_globalData->sceneAABB.max.z},
        };

        float minProj = FLT_MAX, maxProj = -FLT_MAX;
        for (const auto &corner : corners) {
            const float proj = glm::dot(corner, lightDirection);
            minProj = std::min(minProj, proj);
            maxProj = std::max(maxProj, proj);
        }

        const float distance = maxProj - glm::dot(sceneCenter, lightDirection);
        const glm::vec3 lightPosition = sceneCenter - lightDirection * distance;
        directionalLight.directionalLightPosition = lightPosition;

        const glm::vec3 up = glm::abs(glm::dot(lightDirection, glm::vec3(0.f, 1.f, 0.f))) > 0.99f
            ? glm::vec3(0.f, 0.f, 1.f) : glm::vec3(0.f, 1.f, 0.f);

        directionalLight.view = glm::lookAt(lightPosition, sceneCenter, up);

        glm::vec3 minLS(FLT_MAX), maxLS(-FLT_MAX);
        for (const auto &corner : corners) {
            const glm::vec3 tc = glm::vec3(directionalLight.view * glm::vec4(corner, 1.0f));
            minLS = glm::min(minLS, tc);
            maxLS = glm::max(maxLS, tc);
        }

        directionalLight.projection = glm::ortho(minLS.x, maxLS.x, minLS.y, maxLS.y, 0.0f, maxLS.z - minLS.z);
        directionalLight.projection[1][1] *= -1;
    }

    void ShadowPass::createUniformBuffers() {
        m_directionalLightingBuffer = Resources::Buffers::UniformBuffer(
            &m_ctx->bufferManager(), m_ctx->allocator(), sizeof(Scene::DirectionalLightData)
        );
        m_directionalLightingBuffer.update(directionalLight);
    }

    void ShadowPass::createDescriptors() {
        const auto texCount = static_cast<uint32_t>(m_globalData->modelTextures.size());

        Graphics::Descriptors::DescriptorSetLayoutBuilder layoutBuilder(m_ctx->context().device());
        m_descriptorLayout = layoutBuilder
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, texCount)
            .build();

        const std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texCount * MAX_FRAMES_IN_FLIGHT}
        };
        m_descriptorManager = std::make_unique<Graphics::Descriptors::MainDescriptorManager>(
            m_ctx->context().device(), m_descriptorLayout->handle(), poolSizes, MAX_FRAMES_IN_FLIGHT
        );

        using UpdateInfo = Graphics::Descriptors::MainDescriptorManager::DescriptorUpdateInfo;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VkDescriptorBufferInfo bufferInfo{
                .buffer = m_directionalLightingBuffer.handle(),
                .offset = 0,
                .range = sizeof(Scene::DirectionalLightData)
            };
            m_globalData->frameData[i].directionalLightBufferInfo = bufferInfo;

            UpdateInfo ubo{}; ubo.binding = 0; ubo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ubo.bufferInfo = &bufferInfo; ubo.descriptorCount = 1; ubo.isImage = false;

            UpdateInfo textures{}; textures.binding = 1; textures.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            textures.imageInfo = m_globalData->frameData[i].textureImageInfos.data();
            textures.descriptorCount = texCount; textures.isImage = true;

            m_descriptorManager->updateDescriptorSet(i, {ubo, textures});
        }
    }

    void ShadowPass::createPipeline() {
        static constexpr std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.depthAttachmentFormat = m_dependencies->shadowMap->getImage()->format();

        Graphics::Pipeline::PipelineConfig config{};
        config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/shadows_vert.spv";
        config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/shadows_frag.spv";
        config.inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        config.viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                .viewportCount = 1, .scissorCount = 1};
        config.multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
        config.rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                             .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_BACK_BIT,
                             .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                             .depthBiasEnable = VK_TRUE, .depthBiasConstantFactor = 1.25f,
                             .depthBiasSlopeFactor = 1.75f, .lineWidth = 1.0f};
        config.depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                               .depthTestEnable = VK_TRUE, .depthWriteEnable = VK_TRUE,
                               .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL};
        config.dynamicState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                               .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                               .pDynamicStates = dynamicStates.data()};
        config.rendering = renderingInfo;

        m_pipeline = std::make_unique<Graphics::Pipeline::Pipeline>(
            &m_ctx->context(), m_descriptorLayout->handle(), config
        );
    }
}
