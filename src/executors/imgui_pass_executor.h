//
// Created by ivans on 04/05/2025.
//

#ifndef SALAMANDER_IMGUI_PASS_EXECUTOR_H
#define SALAMANDER_IMGUI_PASS_EXECUTOR_H


#include <array>
#include <vector>
#include "render_pass_executor.h"
#include "render_pass.h"
#include "frame/frame_data.h"

namespace Salamander::Executors {
    class ImGuiPassExecutor : public RenderPassExecutor {
    public :
        struct Resources {
            VkExtent2D extent;
            std::vector<VkImageView> swapchainImageViews;
            std::array<VkImageView, Renderer::Frame::MAX_FRAMES_IN_FLIGHT> depthImageViews;
            uint32_t *currentFrame;
        };


        explicit ImGuiPassExecutor (Resources resources);
        ~ImGuiPassExecutor()
        override = default;

        void begin(VkCommandBuffer cmd, uint32_t imageIndex) override;

        void setViewportAndScissor(VkCommandBuffer cmd) const;

        void execute(VkCommandBuffer cmd) override;
        void end(VkCommandBuffer cmd) override;

    private:
        Resources m_resources;
    };
}


#endif //SALAMANDER_IMGUI_PASS_EXECUTOR_H