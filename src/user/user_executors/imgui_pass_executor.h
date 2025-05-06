#pragma once
#include "render_pass_executor.h"
#include "render_pass.h"
#include <imgui.h>
#include <vector>

class ImGuiPassExecutor : public RenderPassExecutor {
public:
    struct Resources {
        VkExtent2D                  extent;
        std::vector<VkImageView>    swapchainImageViews;
        VkImageView                 depthImageView;  // Optional
    };


    explicit ImGuiPassExecutor(Resources resources);
    ~ImGuiPassExecutor() override = default;

    void begin(VkCommandBuffer cmd, uint32_t imageIndex) override;

    void setViewportAndScissor(VkCommandBuffer cmd) const;

    void execute(VkCommandBuffer cmd) override;
    void end(VkCommandBuffer cmd) override;

private:
    Resources m_resources;
};