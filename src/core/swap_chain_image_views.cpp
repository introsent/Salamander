#include "swap_chain_image_views.h"
#include <stdexcept>

SwapChainImageViews::SwapChainImageViews(Context* context, const SwapChain* swapChain)
    : m_context(context), m_format(swapChain->format()) {
    createImageViews(swapChain);
}

SwapChainImageViews::~SwapChainImageViews() {
    cleanup();
}

void SwapChainImageViews::recreate(const SwapChain* swapChain) {
    cleanup();
    m_format = swapChain->format();
    createImageViews(swapChain);
}

void SwapChainImageViews::createImageViews(const SwapChain* swapChain) {
    const auto& images = swapChain->images();
    m_imageViews.resize(images.size());

    for (size_t i = 0; i < images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_context->device(), &createInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

void SwapChainImageViews::cleanup() {
    for (auto imageView : m_imageViews) {
        vkDestroyImageView(m_context->device(), imageView, nullptr);
    }
    m_imageViews.clear();
}
