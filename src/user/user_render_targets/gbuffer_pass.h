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
    void execute(VkCommandBuffer cmd, uint32_t frameIndex) override;

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
    ManagedTexture m_albedoTexture;
    ManagedTexture m_normalTexture;
    ManagedTexture m_paramTexture;
};