#pragma once
#include "context.h"
#include <vector>

class FramebufferManager final {
public:
    FramebufferManager(Context* context,
        const std::vector<VkImageView>* imageViews,
        VkExtent2D extent,
        VkRenderPass renderPass,
        VkImageView depthImageView);
    ~FramebufferManager();
    FramebufferManager(const FramebufferManager&) = delete;
    FramebufferManager& operator=(const FramebufferManager&) = delete;
    FramebufferManager(FramebufferManager&&) = delete;
    FramebufferManager& operator=(FramebufferManager&&) = delete;

    const std::vector<VkFramebuffer>& framebuffers() const { return m_framebuffers; }
    void recreate(const std::vector<VkImageView>* imageViews, VkExtent2D extent,
        VkRenderPass renderPass, VkImageView depthImageView);

private:
    void createFramebuffers(const std::vector<VkImageView>* imageViews,
        VkExtent2D extent,
        VkRenderPass renderPass,
        VkImageView depthImageView);
    void cleanup();

    Context* m_context;
    std::vector<VkFramebuffer> m_framebuffers;
};
