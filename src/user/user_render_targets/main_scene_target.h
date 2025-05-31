#pragma once
#include "target/render_target.h"
#include "main_scene_controller.h"

class MainSceneTarget : public RenderTarget {
public:
    void initialize(const SharedResources& shared) override;
    void render(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
    void recreateSwapChain() override;
    void cleanup() override;
    void updateUniformBuffers() const override;

private:
    MainSceneController m_controller;
};