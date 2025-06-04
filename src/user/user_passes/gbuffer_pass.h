#pragma once
#include "irender_pass.h"
#include "render_pass.h"
#include "pipeline.h"
#include "descriptors/descriptor_set_layout.h"
#include "user_descriptor_managers/main_descriptor_manager.h"

class GBufferPass : public IRenderPass {
public:
    void initialize(const RenderTarget::SharedResources& shared,
                   MainSceneGlobalData& globalData,
                   PassDependencies& dependencies) override;
    void cleanup() override;
    void recreateSwapChain() override;
    void execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) override;

private:
    void createPipeline();
    void createAttachments();
    void createDescriptors();

    // Resources
    const RenderTarget::SharedResources* m_shared = nullptr;
    MainSceneGlobalData* m_globalData = nullptr;
    PassDependencies* m_dependencies = nullptr;
    
    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<DescriptorSetLayout> m_descriptorLayout;
    std::unique_ptr<MainDescriptorManager> m_descriptorManager;
    
    // Attachments
    std::array<ManagedTexture, MAX_FRAMES_IN_FLIGHT> m_depthTextures;
    std::array<ManagedTexture, MAX_FRAMES_IN_FLIGHT> m_albedoTextures;
    std::array<ManagedTexture, MAX_FRAMES_IN_FLIGHT> m_normalTextures;
    std::array<ManagedTexture, MAX_FRAMES_IN_FLIGHT> m_paramTextures;
};