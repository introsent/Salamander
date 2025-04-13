#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

#include "vk_mem_alloc.h"

class CommandManager;
class BufferManager;

struct ManagedTexture {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE; // optional, null for non-sampled textures
};

class TextureManager {
public:
    TextureManager(VkDevice device, VmaAllocator allocator, CommandManager* commandManager, BufferManager* bufferManager);
    ~TextureManager();
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;

    ManagedTexture& loadTexture(const std::string& filepath);
    ManagedTexture& createTexture(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                                  VmaMemoryUsage memoryUsage, VkImageAspectFlags aspect, bool createSampler);

    const std::vector<ManagedTexture>& getTextures() const { return m_managedTextures; }

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

    std::vector<ManagedTexture> m_managedTextures;

};
