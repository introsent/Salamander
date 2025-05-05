// framebuffer_manager.cpp
#include "framebuffer_manager.h"
#include "deletion_queue.h"
#include <array>
#include <stdexcept>

FramebufferManager::FramebufferManager(Context* context,
    const std::vector<VkImageView>* imageViews,
    VkExtent2D extent,
    VkImageView depthImageView)
    : m_context(context)
    , m_imageViews(imageViews)
    , m_extent(extent)
    , m_depthView(depthImageView) {
}

FramebufferManager::FramebufferSet FramebufferManager::createFramebuffersForRenderPass(VkRenderPass renderPass) {
    FramebufferSet set;
    set.framebuffers.resize(m_imageViews->size());

    for (size_t i = 0; i < m_imageViews->size(); i++) {
        std::array<VkImageView, 2> attachments = { (*m_imageViews)[i], m_depthView };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_extent.width;
        framebufferInfo.height = m_extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_context->device(), &framebufferInfo, nullptr, &set.framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }

        // Register cleanup for each framebuffer
        VkDevice device = m_context->device();
        VkFramebuffer framebuffer = set.framebuffers[i];
        DeletionQueue::get().pushFunction(
            "Framebuffer_" + std::to_string(i) + "_" + std::to_string(reinterpret_cast<uint64_t>(renderPass)),
            [device, framebuffer]() {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            });
    }

    m_framebufferSets[renderPass] = set;
    return set;
}

void FramebufferManager::recreate(const std::vector<VkImageView>* imageViews,
    VkExtent2D extent,
    VkImageView depthImageView) {
    cleanup();
    m_imageViews = imageViews;
    m_extent = extent;
    m_depthView = depthImageView;
}

void FramebufferManager::cleanup() {
    for (auto& [renderPass, set] : m_framebufferSets) {
        for (auto framebuffer : set.framebuffers) {
            vkDestroyFramebuffer(m_context->device(), framebuffer, nullptr);
        }
    }
    m_framebufferSets.clear();
}