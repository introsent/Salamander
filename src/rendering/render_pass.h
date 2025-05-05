#pragma once
#include "context.h"
#include <vulkan/vulkan.h>
#include <memory>

// REDUNDANT IF USING DYNAMIC RENDERING
class RenderPass {
public:
    // Configuration structure
    struct Config {
        VkFormat colorFormat;
        VkFormat depthFormat;
        VkAttachmentLoadOp colorLoadOp;
        VkAttachmentStoreOp colorStoreOp;
        VkImageLayout colorInitialLayout;
        VkImageLayout colorFinalLayout;
        VkAttachmentLoadOp depthLoadOp;
        VkAttachmentStoreOp depthStoreOp;
        VkImageLayout depthInitialLayout;
        VkImageLayout depthFinalLayout;
    };

    static std::unique_ptr<RenderPass> create(const Context* context, const Config& config);

    ~RenderPass();

    // No copying/moving
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&) = delete;
    RenderPass& operator=(RenderPass&&) = delete;

    VkRenderPass handle() const { return m_renderPass; }

private:
    RenderPass(const Context* context, const Config& config);
    void initialize();

    const Context* m_context;
    Config m_config;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
};