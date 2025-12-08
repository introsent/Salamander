//
// Created by ivans on 05/12/2025.
//

#include "texture.h"

#include "deletion_queue.h"

static int samplerInx = 0;
static int imageViewInx = 0;

Texture::Texture(VkDevice device)
{
    m_device = device;
}

void Texture::create(std::unique_ptr<Image> img)
{
    m_image = std::move(img);

    m_width = m_image->size().x;
    m_height = m_image->size().y;
    m_format = m_image->format();
    m_mipLevels = m_image->mipLevels();

    createImageView();
    createSampler();
}

void Texture::createImageView()
{
    const VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_image->handle(),
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = m_format,
        .subresourceRange.aspectMask = m_image->aspect(),
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = m_mipLevels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1
    };

    vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView);

    VkDevice deviceCopy = m_device;
    VkImageView imageViewCopy = m_imageView;
    DeletionQueue::get().pushFunction("ImageViewTexture_" + std::to_string(imageViewInx++),
        [deviceCopy, imageViewCopy]() {
        vkDestroyImageView(deviceCopy, imageViewCopy, nullptr);
        });
}

void Texture::createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxLod = static_cast<float>(m_mipLevels);

    vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler);

    VkDevice deviceCopy = m_device;
    VkSampler samplerCopy = m_sampler;
    DeletionQueue::get().pushFunction("Sampler_" + std::to_string(samplerInx++),
        [deviceCopy, samplerCopy]() {
        vkDestroySampler(deviceCopy, samplerCopy, nullptr);
        });
}

VkDescriptorImageInfo Texture::getDescriptorInfo() const
{
    VkDescriptorImageInfo info{};
    info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    info.imageView = m_imageView;
    info.sampler   = m_sampler;
    return info;
}
