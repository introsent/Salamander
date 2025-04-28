#include "framebuffer_manager.h"
#include <array>
#include <stdexcept>

FramebufferManager::FramebufferManager(Context* context,
    const std::vector<VkImageView>* imageViews,
    VkExtent2D extent,
    VkRenderPass renderPass,
    VkImageView depthImageView)
    : m_context(context) {
    createFramebuffers(imageViews, extent, renderPass, depthImageView);
}


void FramebufferManager::recreate(const std::vector<VkImageView>* imageViews,
    VkExtent2D extent,
    VkRenderPass renderPass,
    VkImageView depthImageView) {
    cleanup();
    createFramebuffers(imageViews, extent, renderPass, depthImageView);
}

void FramebufferManager::createFramebuffers(const std::vector<VkImageView>* imageViews,
    VkExtent2D extent,
    VkRenderPass renderPass,
    VkImageView depthImageView) {
    m_framebuffers.resize(imageViews->size());
    for (size_t i = 0; i < imageViews->size(); i++) {
        std::array<VkImageView, 2> attachments = { (*imageViews)[i], depthImageView };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_context->device(), &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }

    // Register a *final cleanup* for shutdown
    for (auto framebuffer : m_framebuffers) {
        VkDevice device = m_context->device(); // capture by value!
        DeletionQueue::get().pushFunction([device, framebuffer]() {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
            });
    }
}

void FramebufferManager::cleanup() {
    for (auto framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(m_context->device(), framebuffer, nullptr);
    }
    m_framebuffers.clear();
}
