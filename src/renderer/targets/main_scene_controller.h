#pragma once
#include "config.h"
#include "cube_map_renderer.h"
#include "user_passes/depth_prepass.h"
#include "user_passes/gbuffer_pass.h"
#include "user_passes/lighting_pass.h"
#include "user_passes/tone_mapping_pass.h"
#include "data_structures.h"
#include "uniform_buffer.h"
#include "user_passes/luminance_average_pass.h"
#include "user_passes/luminance_histogram_pass.h"
#include "user_passes/shadow_pass.h"

namespace Salamander::Renderer::Targets {
    class MainSceneController {
    public:
        void initialize(const SharedResources &shared);
        void cleanup();
        void recreateSwapChain();
        void render(float deltaTime, VkCommandBuffer cmd, uint32_t imageIndex);
        void updateUniformBuffers() const;

    private:
        void loadModel(const std::string &path);
        void createBuffers();
        uint32_t createDefaultMaterialTexture(float metallicFactor, float roughnessFactor);
        void createIBLResources();

        void setPointLightEnabled(bool enabled);

        // Passes
        DepthPrepass m_depthPrepass;
        GBufferPass m_gBufferPass;
        LightingPass m_lightingPass;
        LuminanceHistogramPass m_luminanceHistogramPass;
        LuminanceAveragePass m_luminanceAveragePass;
        ToneMappingPass m_toneMappingPass;
        ShadowPass m_shadowPass;

        // Shared data
        MainSceneGlobalData m_globalData;
        PassDependencies m_dependencies;
        const SharedResources *m_shared = nullptr;

        PointLightData m_pointLightData{};
        bool m_pointLightEnabled = true;

        CubeMapRenderer m_cubeMapRenderer;
        CubeMapRenderer::CubeMap m_envCubeMap;
        CubeMapRenderer::CubeMap m_irradianceMap;
        Texture *m_hdrEquirect;

        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
        const std::string MODEL_PATH = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/Sponza.gltf";

        std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
        std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> m_omniLightBuffer;
        std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> m_cameraExposureBuffer;
    };
}
