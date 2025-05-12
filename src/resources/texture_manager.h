#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <vector>
#include <string>

class BufferManager;
class CommandManager;

struct ManagedTexture {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE; // Optional (only for sampled images)
};

class TextureManager {
public:
    TextureManager(VkDevice device, VmaAllocator allocator,
        CommandManager* commandManager, BufferManager* bufferManager);
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;

    ManagedTexture& loadTexture(const std::string& filepath);
    ManagedTexture& createTexture(uint32_t width, uint32_t height, VkFormat format,
        VkImageUsageFlags usage, VmaMemoryUsage memoryUsage,
        VkImageAspectFlags aspect, bool createSampler = false);

    void transitionSwapChainLayout(VkCommandBuffer cmd, VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask) const;

    const std::vector<ManagedTexture>& getTextures() const { return m_managedTextures; }

private:
    VkDevice m_device;
    VmaAllocator m_allocator;
    CommandManager* m_commandManager;
    BufferManager* m_bufferManager;

    std::vector<ManagedTexture> m_managedTextures;

    void createImage(uint32_t width, uint32_t height, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usage,
        VmaMemoryUsage memoryUsage, VkImage& image, VmaAllocation& allocation) const;

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;
    VkSampler createSampler() const;

    void transitionImageLayout(VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout) const;
    void copyBufferToImage(VkBuffer buffer, VkImage image,
        uint32_t width, uint32_t height) const;
};
