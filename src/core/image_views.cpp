#include "image_views.h"
#include "deletion_queue.h" 
#include <stdexcept>

ImageViews::ImageViews(Context* context, VkFormat format, const std::vector<VkImage>& images)
    : m_context(context), m_format(format) {
    createImageViews(images);
}

void ImageViews::recreate(VkFormat format, const std::vector<VkImage>& images) {
    cleanup(); // immediately destroy old ones
    m_format = format;
    createImageViews(images);
}

void ImageViews::createImageViews(const std::vector<VkImage>& images) {
    const auto& localImages = images;
    m_imageViews.resize(localImages.size());

    for (size_t i = 0; i < images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = localImages[i];
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

    int imageViewIndex = 0;
    for (auto imageView : m_imageViews) {
        VkDevice device = m_context->device();
        DeletionQueue::get().pushFunction("ImageView_" + std::to_string(imageViewIndex++), [device, imageView]() {
            vkDestroyImageView(device, imageView, nullptr);
            });
    }
}

void ImageViews::cleanup() {
    for (auto imageView : m_imageViews) {
        vkDestroyImageView(m_context->device(), imageView, nullptr);
    }
    m_imageViews.clear();
}
