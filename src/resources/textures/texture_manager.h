#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <string>
#include <unordered_map>

#include "debug_messenger.h"
#include "texture.h"

namespace Salamander::Resources::Buffers {
    class BufferManager;
    class CommandManager;
}

namespace Salamander::Resources::Textures
{
    class TextureManager {
    public:
        TextureManager(
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            VmaAllocator allocator,
            Buffers::CommandManager* cmdManager,
            Buffers::BufferManager* bufferManager,
            DebugMessenger* debugMessenger
        );
        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;
        TextureManager(TextureManager&&) = delete;
        TextureManager& operator=(TextureManager&&) = delete;

        Texture& loadTexture(
            const std::string& filepath,
            bool generateMips = false,
            VkFormat           format     = VK_FORMAT_R8G8B8A8_SRGB
        );
        Texture& loadHDRTexture(const std::string& path);

        Texture& createTexture(uint32_t width, uint32_t height, VkFormat format,
            VkImageUsageFlags usage, VmaMemoryUsage memoryUsage,
            VkImageAspectFlags aspect, bool generateMipMaps = false, bool createSampler = false, const std::string& debugName = "");
        Texture& createTexture(const unsigned char* data, uint32_t width, uint32_t height,
                                        uint32_t channels, bool generateMipMaps = false, const std::string& debugName = "");

        Texture& createCubeTexture(uint32_t size, VkFormat format,
                                         VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, bool generateMipMaps = false);


        [[nodiscard]] const std::unordered_map<std::string, std::unique_ptr<Texture>>& getTextures() const { return m_textures; }

        [[nodiscard]] VkSampler getDefaultSampler() const { return m_defaultSampler; }
        [[nodiscard]] VkSampler getCubeSampler() const { return m_cubeSampler; }
        [[nodiscard]] VkSampler getDepthSampler() const { return m_depthSampler; }


        // public destroy methods, mainly for swapchain recreation
        void destroyTexture(Texture& texture);
        void destroyTexture(const std::string& key);

    private:
        VkDevice m_device;
        VkPhysicalDevice m_physicalDevice;
        VmaAllocator m_allocator;
        Buffers::CommandManager* m_commandManager;
        Buffers::BufferManager* m_bufferManager;
        DebugMessenger* m_debugMessenger;

        std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;

        // samplers
        VkSampler m_defaultSampler = VK_NULL_HANDLE;
        VkSampler m_cubeSampler = VK_NULL_HANDLE;
        VkSampler m_depthSampler = VK_NULL_HANDLE;

        void createCommonSamplers();
        void destroyCommonSamplers();

    };
}

