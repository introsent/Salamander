#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <vector>
#include <string>
#include <unordered_map>

#include "debug_messenger.h"
#include "texture.h"

class BufferManager;
class CommandManager;


class TextureManager {
public:
    TextureManager(
        VkDevice device,
        VmaAllocator allocator,
        CommandManager* cmdManager,
        BufferManager* bufferManager,
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
                                     VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);


    [[nodiscard]] const std::unordered_map<std::string, std::unique_ptr<Texture>>& getTextures() const { return m_textures; }


private:
    VkDevice m_device;
    VmaAllocator m_allocator;
    CommandManager* m_commandManager;
    BufferManager* m_bufferManager;
    DebugMessenger* m_debugMessenger;

    std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
};
