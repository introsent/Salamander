//
// Created by ivans on 04/05/2025.
//

#ifndef SALAMANDER_IMGUI_TARGET_H
#define SALAMANDER_IMGUI_TARGET_H


#include "frame/frame_data.h"
#include "renderer/targets/target/render_target.h"
#include "graphics/descriptors/managers/imgui_descriptor_manager.h"

namespace Salamander::Renderer::Targets {
    class ImGuiTarget final : public RenderTarget {
    public:
        void initialize(const Frame::RenderContext &ctx) override;
        void render(float deltaTime, VkCommandBuffer cmd, uint32_t imageIndex, uint32_t frameIndex) override;
        void recreateSwapChain() override;
        void cleanup() override;

        void setExtraUiCallback(const std::function<void(uint32_t)>& callback) {
            m_extraUiCallback = callback;
            if (m_executor) {
                m_executor->setExtraUiCallback(m_extraUiCallback);
            }
        }

    private:
        std::function<void(uint32_t)> m_extraUiCallback;

        void createRenderingResources();
        void createDescriptors();
        void initializeImGui() const;

        std::unique_ptr<Graphics::Descriptors::ImGuiDescriptorManager> m_descriptorManager;
        uint32_t m_currentFrame = 0;
    };
}


#endif //SALAMANDER_IMGUI_TARGET_H