#pragma once
#include "frame/frame_data.h"
#include "renderer/targets/target/render_target.h"
#include "graphics/descriptors/managers/imgui_descriptor_manager.h"

namespace Salamander::Renderer::Targets {
    class ImGuiTarget final : public RenderTarget {
    public:
        void initialize(const Frame::RenderContext &ctx) override;
        void render(float deltaTime, VkCommandBuffer cmd, uint32_t imageIndex) override;
        void recreateSwapChain() override;
        void cleanup() override;
        void updateUniformBuffers() const override {}

    private:
        void createRenderingResources();
        void createDescriptors();
        void initializeImGui() const;

        std::unique_ptr<Graphics::Descriptors::ImGuiDescriptorManager> m_descriptorManager;
        uint32_t m_currentFrame = 0;
    };
}