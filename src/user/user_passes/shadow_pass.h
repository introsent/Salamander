#pragma once
#include "irender_pass.h"
#include "pipeline.h"
#include "uniform_buffer.h"
#include "descriptors/descriptor_set_layout.h"
#include "user_descriptor_managers/main_descriptor_manager.h"

class ShadowPass final : public IRenderPass {

public:
    void initialize(const SharedResources& shared,
                           MainSceneGlobalData& globalData,
                           PassDependencies& dependencies) override;
    void cleanup() override;
    void recreateSwapChain() override;
    void execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) override;
private:
    void createPipeline();
    void createDescriptors();
    void createLightMatrices() const;
    void createUniformBuffers();

    const SharedResources* m_shared = nullptr;
    MainSceneGlobalData* m_globalData = nullptr;
    PassDependencies* m_dependencies = nullptr;

    Texture* m_shadowMapTexture = {};

    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<MainDescriptorManager> m_descriptorManager;
    std::unique_ptr<DescriptorSetLayout> m_descriptorLayout;

    UniformBuffer m_directionalLightingBuffer;
};

