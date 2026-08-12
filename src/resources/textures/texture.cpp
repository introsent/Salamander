//
// Created by ivans on 05/12/2025.
//

#include "texture.h"

#include "deletion_queue.h"

static int samplerInx = 0;
static int imageViewInx = 0;

namespace Salamander::Resources::Textures
{
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
    }

    void Texture::createImageView()
    {
        const VkImageViewCreateInfo viewInfo = setupImageViewInfo(0, m_image->layers());

        vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView);

        VkDevice deviceCopy = m_device;
        VkImageView imageViewCopy = m_imageView;
        DeletionQueue::get().pushFunction("ImageViewTexture_" + std::to_string(imageViewInx++),
            [deviceCopy, imageViewCopy]() {
            vkDestroyImageView(deviceCopy, imageViewCopy, nullptr);
            });
    }

    VkImageViewCreateInfo Texture::setupImageViewInfo(const uint32_t baseArrayLayer, const uint32_t layerCount) const {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_image->handle();
        viewInfo.viewType = (layerCount > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
        viewInfo.subresourceRange.layerCount = layerCount;
        viewInfo.format = m_format;
        viewInfo.subresourceRange.aspectMask = m_image->aspect();
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = m_mipLevels;

        return viewInfo;
    }

    void Texture::setupImageViewsOnArray()  {
        m_layerViews.resize(m_image->layers());

        for (int arrayLayerInx = 0; arrayLayerInx < m_layerViews.size(); ++arrayLayerInx) {
            VkImageView imageView;
            const VkImageViewCreateInfo viewInfo = setupImageViewInfo(arrayLayerInx, 1);

            vkCreateImageView(m_device, &viewInfo, nullptr, &imageView);

            m_layerViews[arrayLayerInx] = imageView;

            VkDevice deviceCopy = m_device;
            VkImageView imageViewCopy = m_layerViews[arrayLayerInx];
            DeletionQueue::get().pushFunction("ImageViewTexture_" + std::to_string(imageViewInx++),
                [deviceCopy, imageViewCopy]() {
                vkDestroyImageView(deviceCopy, imageViewCopy, nullptr);
                });
        }
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
        m_ownsSampler = true;

        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler);

        VkDevice deviceCopy = m_device;
        VkSampler samplerCopy = m_sampler;
        DeletionQueue::get().pushFunction("Sampler_" + std::to_string(samplerInx++),
            [deviceCopy, samplerCopy]() {
            vkDestroySampler(deviceCopy, samplerCopy, nullptr);
            });
    }

    void Texture::createCubeImageView()
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_image->handle();
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = m_format;
        viewInfo.subresourceRange.aspectMask = m_image->aspect();
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = m_mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6; // cube map has 6 layers

        if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cube image view!");
        }


        VkDevice deviceCopy = m_device;
        VkImageView imageViewCopy = m_imageView;
        DeletionQueue::get().pushFunction("ImageViewTexture_" + std::to_string(imageViewInx++),
            [deviceCopy, imageViewCopy]() {

            vkDestroyImageView(deviceCopy, imageViewCopy, nullptr);
            });
    }

    void Texture::createCubeSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(m_mipLevels);

        if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cube sampler!");
        }

        VkDevice deviceCopy = m_device;
        VkSampler samplerCopy = m_sampler;
        DeletionQueue::get().pushFunction("Sampler_" + std::to_string(samplerInx++),
            [deviceCopy, samplerCopy]() {
            vkDestroySampler(deviceCopy, samplerCopy, nullptr);
            });
    }

    void Texture::createDepthSampler(bool useComparison) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        // Mipmap settings
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(m_mipLevels);
        samplerInfo.mipLodBias = 0.0f;

        // Anisotropy typically not needed for depth
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

        // If using for shadow mapping with PCF
        if (useComparison) {
            samplerInfo.compareEnable = VK_TRUE;
            samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        } else {
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        }

        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create depth sampler!");
        }

        VkDevice deviceCopy = m_device;
        VkSampler samplerCopy = m_sampler;
        DeletionQueue::get().pushFunction("DepthSampler_" + std::to_string(samplerInx++),
            [deviceCopy, samplerCopy]() {
                vkDestroySampler(deviceCopy, samplerCopy, nullptr);
            });
    }

    void Texture::setDebugName(const Core::DebugMessenger* debug, const std::string& name)
    {
        if (!debug) return;

        if (m_image) {
            debug->setObjectName(
                reinterpret_cast<uint64_t>(m_image->handle()),
                VK_OBJECT_TYPE_IMAGE,
                (name + "_Image").c_str()
            );
        }

        if (m_imageView != VK_NULL_HANDLE) {
            debug->setObjectName(
                reinterpret_cast<uint64_t>(m_imageView),
                VK_OBJECT_TYPE_IMAGE_VIEW,
                (name + "_ImageView").c_str()
            );
        }

        if (m_sampler != VK_NULL_HANDLE) {
            debug->setObjectName(
                reinterpret_cast<uint64_t>(m_sampler),
                VK_OBJECT_TYPE_SAMPLER,
                (name + "_Sampler").c_str()
            );
        }
    }

    VkDescriptorImageInfo Texture::getDescriptorInfo() const
    {
        VkDescriptorImageInfo info{};
        info.imageLayout = m_image->currentLayout();
        info.imageView = m_imageView;
        info.sampler   = m_sampler;
        return info;
    }
}

