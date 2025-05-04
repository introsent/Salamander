#pragma once
#include "../rendering/target/render_target.h"
#include "../rendering/pipeline.h"
#include "../rendering/descriptors/descriptor_set_layout.h"
#include "../resources/vertex_buffer.h"
#include "../resources/index_buffer.h"
#include "../resources/uniform_buffer.h"
#include "../user/user_descriptor_managers/imgui_descriptor_manager.h"

class ImGuiTarget : public RenderTarget {
public:
    void initialize(const SharedResources& shared) override;
    void render(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
    void recreateSwapChain() override;
    void cleanup() override;

private:
    void createRenderingResources();
    void createDescriptors();
    void initializeImGui();

    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<ImGuiDescriptorManager> m_descriptorManager;
};