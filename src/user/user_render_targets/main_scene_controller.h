#pragma once
#include "config.h"
#include "cube_map_renderer.h"
#include "user_passes/depth_prepass.h"
#include "user_passes/gbuffer_pass.h"
#include "user_passes/lighting_pass.h"
#include "user_passes/tone_mapping_pass.h"
#include "data_structures.h"
#include "uniform_buffer.h"

class MainSceneController {
public:
    void initialize(const RenderTarget::SharedResources& shared);
    void cleanup();
    void recreateSwapChain();
    void render(VkCommandBuffer cmd, uint32_t imageIndex);
    void updateUniformBuffers() const;

private:
    void createSamplers();
    void loadModel(const std::string& path);
    void createBuffers();
    uint32_t createDefaultMaterialTexture(float metallicFactor, float roughnessFactor);
    void createIBLResources();

    // Passes
    DepthPrepass m_depthPrepass;
    GBufferPass m_gBufferPass;
    LightingPass m_lightingPass;
    ToneMappingPass m_toneMappingPass;

    // Shared data
    MainSceneGlobalData m_globalData;
    PassDependencies m_dependencies;
    const RenderTarget::SharedResources* m_shared = nullptr;

    CubeMapRenderer m_cubeMapRenderer;
    CubeMapRenderer::CubeMap m_envCubeMap;
    ManagedTexture m_hdrEquirect;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    const std::string MODEL_PATH = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/Sponza.gltf";

    // Additional buffers needed
    std::vector<UniformBuffer> m_uniformBuffers;
    UniformBuffer m_omniLightBuffer;
    UniformBuffer m_cameraExposureBuffer;
};
