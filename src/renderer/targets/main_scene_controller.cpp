#include "main_scene_controller.h"
#include <iostream>
#include <vk_mem_alloc.h>
#include "camera/camera_exposure.h"
#include "graphics/image_transition_manager.h"
#include "textures/texture_manager.h"

#ifdef USE_TINYGLTF
    #include "loaders/gltf_loader.h"
#endif
#ifdef USE_ASSIMP
    #include "loaders/assimp_loader.h"
#endif

namespace Salamander::Renderer::Targets {

    void MainSceneController::initialize(const Frame::RenderContext &ctx) {
        m_ctx = &ctx;
        loadModel(MODEL_PATH);
        createBuffers();

        constexpr uint32_t SHADOW_MAP_SIZE = 4096;
        m_dependencies.shadowMap = &ctx.textureManager().createTexture(
            SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            false, true, "ShadowMapTexture"
        );

        m_cubeMapRenderer.initialize(
            &ctx.context(),
            &ctx.bufferManager(),
            &ctx.textureManager()
        );

        m_shadowPass.initialize(ctx, m_globalData, m_dependencies);
        createIBLResources();

        m_depthPrepass.initialize(ctx, m_globalData, m_dependencies);
        m_gBufferPass.initialize(ctx, m_globalData, m_dependencies);
        m_lightingPass.initialize(ctx, m_globalData, m_dependencies);
        m_luminanceHistogramPass.initialize(ctx, m_globalData, m_dependencies);
        m_luminanceAveragePass.initialize(ctx, m_globalData, m_dependencies);
        m_toneMappingPass.initialize(ctx, m_globalData, m_dependencies);
    }

    void MainSceneController::cleanup() {
        vkDeviceWaitIdle(m_ctx->context().device());
        m_toneMappingPass.cleanup();
        m_luminanceAveragePass.cleanup();
        m_luminanceHistogramPass.cleanup();
        m_lightingPass.cleanup();
        m_gBufferPass.cleanup();
        m_depthPrepass.cleanup();
    }

    void MainSceneController::recreateSwapChain() {
        auto &frames = m_ctx->frames();
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkWaitForFences(m_ctx->context().device(), 1,
                            &frames[i].inFlightFence, VK_TRUE, UINT64_MAX);
        }
        m_depthPrepass.recreateSwapChain();
        m_gBufferPass.recreateSwapChain();
        m_lightingPass.recreateSwapChain();
        m_luminanceHistogramPass.recreateSwapChain();
        m_luminanceAveragePass.recreateSwapChain();
        m_toneMappingPass.recreateSwapChain();
    }

    void MainSceneController::render(float deltaTime, VkCommandBuffer cmd, uint32_t imageIndex) {
        // Light toggle (L key)
        static bool wasPressed = false;
        if (glfwGetKey(m_ctx->window().handle(), GLFW_KEY_L) == GLFW_PRESS) {
            if (!wasPressed) {
                m_pointLightEnabled = !m_pointLightEnabled;
                setPointLightEnabled(m_pointLightEnabled);
            }
            wasPressed = true;
        } else {
            wasPressed = false;
        }

        m_globalData.deltaTime = deltaTime;
        updateUniformBuffers();

        // Determine current frame index from the frames vector
        // (assumes caller passes the correct imageIndex matching current frame)
        const uint32_t frameIndex = imageIndex % MAX_FRAMES_IN_FLIGHT;

        Graphics::ImageTransitionManager::transitionColorAttachment(
            cmd,
            m_ctx->swapChain().getCurrentImage(imageIndex),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );

        m_depthPrepass.execute(cmd, frameIndex, imageIndex);
        m_gBufferPass.execute(cmd, frameIndex, imageIndex);
        m_lightingPass.execute(cmd, frameIndex, imageIndex);
        m_luminanceHistogramPass.execute(cmd, frameIndex, imageIndex);
        m_luminanceAveragePass.execute(cmd, frameIndex, imageIndex);
        m_toneMappingPass.execute(cmd, frameIndex, imageIndex);

        Graphics::ImageTransitionManager::transitionToPresent(
            cmd,
            m_ctx->swapChain().getCurrentImage(imageIndex),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );
    }

    void MainSceneController::updateUniformBuffers() const {
        Scene::UniformBufferObject ubo{};
        ubo.model    = glm::mat4(1.0f);
        ubo.view     = m_ctx->camera().GetViewMatrix();
        ubo.proj     = m_ctx->camera().GetProjectionMatrix(
            static_cast<float>(m_ctx->swapChain().extent().width) /
            static_cast<float>(m_ctx->swapChain().extent().height)
        );
        ubo.cameraPosition = m_ctx->camera().Position;

        // currentFrame index needs to come from frame tracking —
        // keep a mutable currentFrame member or pass it in; here we update all frames
        // if you have a currentFrame pointer, use that index only.
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            m_uniformBuffers[i].update(ubo);
        }
    }

    // -------------------------------------------------------------------------
    // Private helpers — loadModel, createBuffers, createIBLResources,
    // setPointLightEnabled are identical in logic to the original; only the
    // resource access is updated to go through m_ctx-> instead of m_shared->
    // -------------------------------------------------------------------------

    void MainSceneController::createIBLResources() {
        m_hdrEquirect = &m_ctx->textureManager().loadHDRTexture(
            std::string(SOURCE_RESOURCE_DIR) + "/textures/circus_arena.hdr"
        );

        m_envCubeMap = m_cubeMapRenderer.createCubeMap(1024, VK_FORMAT_R16G16B16A16_SFLOAT);

        VkCommandBuffer cmd = m_ctx->commandManager().beginSingleTimeCommands();
        m_cubeMapRenderer.renderEquirectToCube(cmd, m_hdrEquirect, m_envCubeMap);
        m_irradianceMap = m_cubeMapRenderer.createDiffuseIrradianceMap(cmd, m_envCubeMap, 128);
        m_shadowPass.execute(cmd, 0, 0);
        m_ctx->commandManager().endSingleTimeCommands(cmd);

        m_dependencies.equirectTexture = m_hdrEquirect;
        m_dependencies.cubeMap         = m_envCubeMap.texture;
        m_dependencies.irradianceMap   = m_irradianceMap.texture;
    }

    void MainSceneController::setPointLightEnabled(bool enabled) {
        m_pointLightData.enabled = enabled ? 1 : 0;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            m_omniLightBuffer[i].update(m_pointLightData);
        }
    }

    void MainSceneController::createBuffers() {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            // UBO
            VkDeviceSize uboSize = sizeof(Scene::UniformBufferObject);
            m_uniformBuffers[i] = Resources::Buffers::UniformBuffer(
                &m_ctx->bufferManager(), m_ctx->allocator(), uboSize
            );
            m_globalData.frameData[i].bufferInfo = {
                .buffer = m_uniformBuffers[i].handle(),
                .offset = 0,
                .range  = uboSize
            };

            // Point light
            VkDeviceSize lightSize = sizeof(Scene::PointLightData);
            m_omniLightBuffer[i] = Resources::Buffers::UniformBuffer(
                &m_ctx->bufferManager(), m_ctx->allocator(), lightSize
            );
            Scene::PointLightData lightData{};
            lightData.pointLightPosition  = glm::vec3(9.0f, 2.0f, -1.0f);
            lightData.pointLightIntensity = 50.f;
            lightData.pointLightColor     = glm::vec3(1.f, 0.f, 0.f);
            lightData.pointLightRadius    = 5.f;
            lightData.enabled             = 1;
            m_omniLightBuffer[i].update(lightData);
            m_globalData.frameData[i].omniLightBufferInfo = {
                .buffer = m_omniLightBuffer[i].handle(),
                .offset = 0,
                .range  = lightSize
            };
            m_pointLightData = lightData;

            // Camera exposure
            constexpr VkDeviceSize exposureSize = sizeof(Scene::CameraExposure);
            m_cameraExposureBuffer[i] = Resources::Buffers::UniformBuffer(
                &m_ctx->bufferManager(), m_ctx->allocator(), exposureSize
            );
            m_cameraExposureBuffer[i].update(camExpUBO);
            m_globalData.frameData[i].cameraExposureBufferInfo = {
                .buffer = m_cameraExposureBuffer[i].handle(),
                .offset = 0,
                .range  = exposureSize
            };
        }

        // Descriptor image infos
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            auto &fd = m_globalData.frameData[i];

            fd.textureImageInfos.clear();
            for (const auto *tex : m_globalData.modelTextures)
                if (tex) fd.textureImageInfos.push_back(tex->getDescriptorInfo());

            fd.materialImageInfos.clear();
            for (const auto *tex : m_globalData.materialTextures)
                if (tex) fd.materialImageInfos.push_back(tex->getDescriptorInfo());

            fd.normalImageInfos.clear();
            for (const auto *tex : m_globalData.normalTextures)
                if (tex) fd.normalImageInfos.push_back(tex->getDescriptorInfo());
        }
    }

    uint32_t MainSceneController::createDefaultMaterialTexture(
        float metallicFactor, float roughnessFactor)
    {
        unsigned char data[4] = {
            0,
            static_cast<unsigned char>(roughnessFactor * 255),
            static_cast<unsigned char>(metallicFactor  * 255),
            255
        };
        std::string debugName = "default_material_tex_" +
                                std::to_string(m_globalData.materialTextures.size());
        auto &tex = m_ctx->textureManager().createTexture(data, 1, 1, 4, false, debugName);
        m_globalData.materialTextures.push_back(&tex);
        return static_cast<uint32_t>(m_globalData.materialTextures.size() - 1);
    }

} // namespace Salamander::Renderer::Targets