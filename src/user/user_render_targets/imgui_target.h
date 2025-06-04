#pragma once
#include "target/render_target.h"
#include "pipeline.h"
#include "descriptors/descriptor_set_layout.h"
#include "user_descriptor_managers/imgui_descriptor_manager.h"

class ImGuiTarget : public RenderTarget {
public:
    void initialize(const SharedResources& shared) override;
    void render(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
    void recreateSwapChain() override;
    void cleanup() override;
    void updateUniformBuffers() const override {};

private:
    void createRenderingResources();
    void createDescriptors();
    void initializeImGui() const;

    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<ImGuiDescriptorManager> m_descriptorManager;
};