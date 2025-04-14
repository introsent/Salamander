#pragma once
#include "../core/context.h"

class RenderPass {
public:
    RenderPass(const Context* context, VkFormat swapChainFormat, VkFormat depthFormat);
    ~RenderPass();
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&) = delete;
    RenderPass& operator=(RenderPass&&) = delete;

    VkRenderPass handle() const { return m_renderPass; }

private:
    const Context* m_context;
    VkRenderPass m_renderPass;

    void createRenderPass(VkFormat swapChainFormat, VkFormat depthFormat);
};

