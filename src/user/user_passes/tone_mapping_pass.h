#pragma once
#include "render_pass.h"
#include "pipeline.h"
#include "descriptors/descriptor_set_layout.h"
#include "user_descriptor_managers/main_descriptor_manager.h"
#include "irender_pass.h"

class ToneMappingPass : public IRenderPass {
public:
    void initialize(const RenderTarget::SharedResources& shared,
                   MainSceneGlobalData& globalData,
                   PassDependencies& dependencies) override;
    void cleanup() override;
    void recreateSwapChain() override;
    void execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) override;

private:
    void createPipeline();
    void createDescriptors();
    void updateDescriptors() const;

    // Resources
    const RenderTarget::SharedResources* m_shared = nullptr;
    MainSceneGlobalData* m_globalData = nullptr;
    PassDependencies* m_dependencies = nullptr;
    
    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<DescriptorSetLayout> m_descriptorLayout;
    std::unique_ptr<MainDescriptorManager> m_descriptorManager;

    std::vector<VkImageLayout> m_currentColorLayouts;
};
