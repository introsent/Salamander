#pragma once
#include "../core/context.h"
#include <vulkan/vulkan.h>

class RenderPassBuilder;  // Forward declaration

class RenderPass {
public:
    RenderPass(const Context* context, VkFormat swapChainFormat, VkFormat depthFormat);

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&) = delete;
    RenderPass& operator=(RenderPass&&) = delete;

    VkRenderPass handle() const { return m_renderPass; }

private:
    friend class RenderPassBuilder;  // Allow builder to access private members

    const Context* m_context;
    VkRenderPass m_renderPass;

    void createRenderPass(VkFormat swapChainFormat, VkFormat depthFormat);
};

class RenderPassBuilder {
public:
    RenderPassBuilder(const Context* context, VkFormat swapChainFormat, VkFormat depthFormat);
    VkRenderPass build();

private:
    const Context* m_context;
    VkFormat m_swapChainFormat;
    VkFormat m_depthFormat;

    struct AttachmentSetups {
        VkAttachmentDescription color;
        VkAttachmentDescription depth;
        VkAttachmentReference colorRef;
        VkAttachmentReference depthRef;
        VkSubpassDescription subpass;
        VkSubpassDependency dependency;
    };

    AttachmentSetups createBaseAttachments();
};