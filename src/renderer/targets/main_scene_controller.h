//
// Created by ivans on 31/05/2025.
//

#ifndef SALAMANDER_MAIN_SCENE_CONTROLLER_H
#define SALAMANDER_MAIN_SCENE_CONTROLLER_H


#include "config.h"
#include "lighting/lights.h"
#include "passes/gbuffer_pass.h"
#include "passes/lighting_pass.h"
#include "passes/luminance_average_pass.h"
#include "passes/luminance_histogram_pass.h"
#include "passes/shadow_pass.h"
#include "passes/tone_mapping_pass.h"
#include "renderer/targets/cube_map_renderer.h"
#include "renderer/passes/depth_prepass.h"
#include "resources/buffers/uniform_buffer.h"
#include "renderer/frame/render_context.h"
#include "render_graph/graph.h"

namespace Salamander::Renderer::Targets {
    class MainSceneController {
    public:
        explicit MainSceneController(Passes::PassDependencies& dependencies);

        void initialize(const Frame::RenderContext &ctx);
        void cleanup();
        void recreateSwapChain();
        void render(float deltaTime, VkCommandBuffer cmd, uint32_t imageIndex, uint32_t frameIndex);
        void updateUniformBuffers(uint32_t frameIndex) const;

    private:
        void loadModel(const std::string &path);
        void createBuffers();
        uint32_t createDefaultMaterialTexture(float metallicFactor, float roughnessFactor);
        void createIBLResources();
        void setPointLightEnabled(bool enabled);

        void updateDirectionalLightMatrices();

        void testRenderGraph();

        RenderGraph::Graph m_renderGraph = {};

        // Passes
        Passes::DepthPrepass m_depthPrepass;
        Passes::GBufferPass m_gBufferPass;
        Passes::LightingPass m_lightingPass;
        Passes::LuminanceHistogramPass m_luminanceHistogramPass;
        Passes::LuminanceAveragePass m_luminanceAveragePass;
        Passes::ToneMappingPass m_toneMappingPass;
        Passes::ShadowPass m_shadowPass;

        // Scene & dependency data
        Scene::MainSceneData m_globalData = {};
        Passes::PassDependencies& m_dependencies;

        const Frame::RenderContext* m_ctx = nullptr;

        // IBL resources
        CubeMapRenderer m_cubeMapRenderer;
        CubeMapRenderer::CubeMap m_envCubeMap;
        CubeMapRenderer::CubeMap m_irradianceMap;
        Resources::Textures::Texture* m_hdrEquirect = nullptr;

        // Per-frame uniform buffers
        static constexpr int MAX_FRAMES_IN_FLIGHT = Frame::MAX_FRAMES_IN_FLIGHT;
        std::array<Resources::Buffers::UniformBuffer, MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
        std::array<Resources::Buffers::UniformBuffer, MAX_FRAMES_IN_FLIGHT> m_omniLightBuffer;
        std::array<Resources::Buffers::UniformBuffer, MAX_FRAMES_IN_FLIGHT> m_cameraExposureBuffer;
        std::array<Resources::Buffers::UniformBuffer, MAX_FRAMES_IN_FLIGHT> m_directionalLightBuffer;

        Scene::PointLightData m_pointLightData{};
        Scene::DirectionalLightData m_directionalLightData{};
        bool m_pointLightEnabled = true;

        const std::string MODEL_PATH =
            std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/Sponza.gltf";
    };
}

#endif //SALAMANDER_MAIN_SCENE_CONTROLLER_H