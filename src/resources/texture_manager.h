#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <vector>
#include <string>

#include "debug_messenger.h"

class BufferManager;
class CommandManager;

struct ManagedTexture {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE; // Optional (only for sampled images)
    std::string id; // Unique identifier for recreation
    uint32_t width = 0; // Store dimensions for debugging
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags usage = 0;
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_UNKNOWN;
    VkImageAspectFlags aspect = 0;
    bool hasSampler = false;
};


class TextureManager {
public:
    TextureManager(VkDevice device, VmaAllocator allocator,
        CommandManager* commandManager, BufferManager* bufferManager, DebugMessenger* debugMessenger);
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;

    ManagedTexture& loadTexture(
        const std::string& filepath,
        VkFormat           format     = VK_FORMAT_R8G8B8A8_SRGB
    );
    ManagedTexture& loadHDRTexture(const std::string& path);

    ManagedTexture& createTexture(uint32_t width, uint32_t height, VkFormat format,
        VkImageUsageFlags usage, VmaMemoryUsage memoryUsage,
        VkImageAspectFlags aspect, bool createSampler = false);
    ManagedTexture& createTexture(const unsigned char* data, uint32_t width, uint32_t height, uint32_t channels);

    ManagedTexture& createCubeTexture(uint32_t size, VkFormat format,
                                     VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
    void createImage(uint32_t width, uint32_t height, VkFormat format,
                                 VkImageTiling tiling, VkImageUsageFlags usage,
                                 VmaMemoryUsage memoryUsage, VkImage& image, VmaAllocation& allocation,
                                 uint32_t layers, VkImageCreateFlags flags) const;


    void transitionSwapChainLayout(VkCommandBuffer cmd, VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask) const;

    const std::vector<ManagedTexture>& getTextures() const { return m_managedTextures; }

    // Static method to access samplerIndex
    static int getSamplerIndex() { return samplerIndex; }
    static void incrementSamplerIndex() { samplerIndex++; }

private:
    VkDevice m_device;
    VmaAllocator m_allocator;
    CommandManager* m_commandManager;
    BufferManager* m_bufferManager;
    DebugMessenger* m_debugMessenger;

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

    static int samplerIndex;
};
