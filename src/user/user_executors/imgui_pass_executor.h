#pragma once
#include "render_pass_executor.h"
#include "render_pass.h"
#include <imgui.h>
#include <vector>

class ImGuiPassExecutor : public RenderPassExecutor {
public:
    struct Resources {
        std::vector<VkFramebuffer>& framebuffers;
        VkExtent2D extent;
        ImGuiContext* imguiContext;
    };

    ImGuiPassExecutor(RenderPass* renderPass, Resources resources);
    ~ImGuiPassExecutor() override = default;

    void begin(VkCommandBuffer cmd, uint32_t imageIndex) override;
    void execute(VkCommandBuffer cmd) override;
    void end(VkCommandBuffer cmd) override;

private:
    RenderPass* m_renderPass;
    Resources m_resources;
};