#pragma once
#include "renderer/targets/target/render_target.h"
#include "main_scene_controller.h"

namespace Salamander::Renderer::Targets {
    class MainSceneTarget final : public RenderTarget {
    public:
        void initialize(const Frame::RenderContext &ctx) override;
        void render(float deltaTime, VkCommandBuffer cmd, uint32_t imageIndex) override;
        void recreateSwapChain() override;
        void cleanup() override;
        void updateUniformBuffers() const override;

    private:
        MainSceneController m_controller;
    };
}