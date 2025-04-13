#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "vk_mem_alloc.h"

class CommandManager;
class BufferManager;

class TextureManager {
public:
    TextureManager(VkDevice device, VmaAllocator allocator, CommandManager* commandManager, BufferManager* bufferManager);
    ~TextureManager();
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;

    void loadTexture(const std::string& filepath);

    VkImageView imageView() const { return m_imageView; }
    VkSampler sampler() const { return m_sampler; }
    VkImage image() const { return m_image; }

 

private:
    void createImage(uint32_t width, uint32_t height, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usage,
        VmaMemoryUsage memoryUsage, VkImage& image, VmaAllocation& allocation) const;

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) const;
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;
    void createSampler();

    VkDevice m_device;
    VmaAllocator m_allocator;
    CommandManager* m_commandManager;
    BufferManager* m_bufferManager;

    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = nullptr;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
};
