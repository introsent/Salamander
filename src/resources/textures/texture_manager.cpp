#include "texture_manager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdexcept>
#include "deletion_queue.h"
#include "buffers/buffer_manager.h"
#include "buffers/command_manager.h"

namespace Salamander::Resources::Textures
{
    TextureManager::TextureManager(VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator allocator,
                                Buffers::CommandManager* cmdManager,
                                Buffers::BufferManager* bufferManager, Core::DebugMessenger* debugMessenger) :
        m_device(device), m_physicalDevice(physicalDevice),m_allocator(allocator), m_commandManager(cmdManager), m_bufferManager(bufferManager),
        m_debugMessenger(debugMessenger)
    {
        createCommonSamplers();
    }

    Texture& TextureManager::loadTexture(const std::string& filepath, bool generateMips, VkFormat format)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(
            filepath.c_str(), &texWidth, &texHeight, &texChannels,
            STBI_rgb_alpha
        );
        if (!pixels) {
            throw std::runtime_error("Failed to load texture image!");
        }

        VkDeviceSize imageSize = texWidth * texHeight * 4;

        // Staging buffer (unchanged)
        Buffers::ManagedBuffer staging = m_bufferManager->createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );
        void* data;
        vmaMapMemory(m_allocator, staging.allocation, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vmaUnmapMemory(m_allocator, staging.allocation);
        stbi_image_free(pixels);

        const uint32_t mipLevels = generateMips
           ? static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1
           : 1;


        // Create the GPU image with the supplied format
        auto image = std::make_unique<Image>(m_allocator);
        image->create(texWidth, texHeight,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY, 1, 0, mipLevels);

        // Transition, copy, and transition again
        VkCommandBuffer cmd = m_commandManager->beginSingleTimeCommands();
        image->transitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        image->copyFromBuffer(cmd, staging.buffer);
        if (!generateMips) {
            image->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        } // else it happens in image->generateMipmaps
        m_commandManager->endSingleTimeCommands(cmd);

        if (generateMips) {
            // generate mip maps
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(m_physicalDevice, image->format(), &formatProperties);
            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
                throw std::runtime_error("texture image format does not support linear blitting!");
            }

            VkCommandBuffer cmdMips = m_commandManager->beginSingleTimeCommands();
            image->generateMipmaps(cmdMips);
            m_commandManager->endSingleTimeCommands(cmdMips);
        }

        // Create texture based on image
        auto texture = std::make_unique<Texture>(m_device);
        texture->create(std::move(image)); // image view and sampler creation already in create
        texture->createImageView();
        texture->setSampler(m_defaultSampler);

        m_textures.insert({filepath, std::move(texture)});
        return *m_textures.at(filepath);
    }

    Texture& TextureManager::loadHDRTexture(const std::string& path) {
        int width, height, channels;
        float* pixels = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("Failed to load HDR image: " + path);
        }

        VkDeviceSize imageSize = width * height * 4 * sizeof(float);

        Buffers::ManagedBuffer staging = m_bufferManager->createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        void* data;
        vmaMapMemory(m_allocator, staging.allocation, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vmaUnmapMemory(m_allocator, staging.allocation);
        stbi_image_free(pixels);

        // Create the GPU image with the supplied format
        VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
        auto image = std::make_unique<Image>(m_allocator);
        image->create(width, height,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY, 1, 0, 1);

        VkCommandBuffer cmd = m_commandManager->beginSingleTimeCommands();
        image->transitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        image->copyFromBuffer(cmd, staging.buffer);
        image->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_commandManager->endSingleTimeCommands(cmd);

        // Create texture based on image
        auto texture = std::make_unique<Texture>(m_device);
        texture->create(std::move(image)); // image view and sampler creation already in create
        texture->createImageView();
        texture->setSampler(m_defaultSampler);

        m_textures.insert({path, std::move(texture)});
        return *m_textures.at(path);
    }

    Texture& TextureManager::createTexture(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
        VmaMemoryUsage memoryUsage, VkImageAspectFlags aspect, bool generateMipMap, bool createSampler,
        const std::string& debugName)
    {
        const uint32_t mipLevels = generateMipMap
            ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1
            : 1;
        auto image = std::make_unique<Image>(m_allocator);
        image->create(width, height, format,
                      VK_IMAGE_TILING_OPTIMAL,
                      usage, memoryUsage,
                      1, 0,  mipLevels);

        auto texture = std::make_unique<Texture>(m_device);
        texture->create(std::move(image));
        texture->createImageView();
        if (createSampler)
        {
            if (format == VK_FORMAT_D32_SFLOAT) {
                texture->setSampler(m_depthSampler);
            }
            else
            {
                texture->setSampler(m_defaultSampler);
            }

        }

        if (!debugName.empty()) {
            texture->setDebugName(m_debugMessenger, debugName);
        }

        m_textures.insert({debugName, std::move(texture)});
        return *m_textures.at(debugName);
    }

    Texture& TextureManager::createTexture(const unsigned char* data, uint32_t width, uint32_t height,
                    uint32_t channels, bool generateMipMaps, const std::string&  debugName)
    {

        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
        if (channels == 1) format = VK_FORMAT_R8_UNORM;
        else if (channels == 2) format = VK_FORMAT_R8G8_UNORM;
        else if (channels == 3) format = VK_FORMAT_R8G8B8_SRGB;

        VkDeviceSize imageSize = width * height * channels;

        Buffers::ManagedBuffer staging = m_bufferManager->createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        void* mapped;
        vmaMapMemory(m_allocator, staging.allocation, &mapped);
        memcpy(mapped, data, static_cast<size_t>(imageSize));
        vmaUnmapMemory(m_allocator, staging.allocation);

        const uint32_t mipLevels = generateMipMaps
            ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1
            : 1;

        auto image = std::make_unique<Image>(m_allocator);
        image->create(width, height, format,
                      VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VMA_MEMORY_USAGE_GPU_ONLY, 1, 0, mipLevels);

        VkCommandBuffer cmd = m_commandManager->beginSingleTimeCommands();
        image->transitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        image->copyFromBuffer(cmd, staging.buffer);
        image->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_commandManager->endSingleTimeCommands(cmd);

        auto texture = std::make_unique<Texture>(m_device);
        texture->create(std::move(image));
        texture->createImageView();
        texture->setSampler(m_defaultSampler);

        if (!debugName.empty()) {
            texture->setDebugName(m_debugMessenger, debugName);
        }

        m_textures.insert({debugName, std::move(texture)});
        return *m_textures.at(debugName);
    }

    Texture& TextureManager::createCubeTexture(uint32_t size, VkFormat format,
                                                    VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, bool generateMipMaps)
    {
        auto image = std::make_unique<Image>(m_allocator);
        const uint32_t mipLevels = generateMipMaps
            ? static_cast<uint32_t>(std::floor(std::log2(std::max(size, size)))) + 1
            : 1;
        image->create(size, size,
                      format,
                      VK_IMAGE_TILING_OPTIMAL,
                      usage,
                      memoryUsage,
                      6, // layers for cubemap
                      VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, // flags if needed
                      1  // mip levels
        );

        auto texture = std::make_unique<Texture>(m_device);
        texture->create(std::move(image));
        texture->createCubeImageView();
        texture->setSampler(m_cubeSampler);

        texture->setDebugName(m_debugMessenger, "CubeMap");

        std::string key = "CubeTexture_" + std::to_string(m_textures.size());
        m_textures.insert({ key, std::move(texture) });

        return *m_textures.at(key);
    }

    Texture& TextureManager::createTextureArray(uint32_t width, uint32_t height, int layerCount, VkFormat format,
        VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImageAspectFlags aspect, bool generateMipMaps,
        bool createSampler, const std::string &debugName) {
        const uint32_t mipLevels = generateMipMaps
            ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1
            : 1;
        auto image = std::make_unique<Image>(m_allocator);
        image->create(width, height, format,
                      VK_IMAGE_TILING_OPTIMAL,
                      usage, memoryUsage,
                      layerCount, 0,  mipLevels);

        auto texture = std::make_unique<Texture>(m_device);
        texture->create(std::move(image));
        texture->createImageView();
        texture->setupImageViewsOnArray();
        if (createSampler)
        {
            if (format == VK_FORMAT_D32_SFLOAT) {
                texture->setSampler(m_depthSampler);
            }
            else
            {
                texture->setSampler(m_defaultSampler);
            }

        }

        if (!debugName.empty()) {
            texture->setDebugName(m_debugMessenger, debugName);
        }

        m_textures.insert({debugName, std::move(texture)});
        return *m_textures.at(debugName);
    }

    void TextureManager::destroyTexture(Texture &texture) {
        for (auto it = m_textures.begin(); it != m_textures.end(); ++it) {
            if (it->second.get() == &texture) {
                m_textures.erase(it);
                return;
            }
        }
    }

    void TextureManager::destroyTexture(const std::string &key) {
        auto it = m_textures.find(key);
        if (it != m_textures.end()) {
            m_textures.erase(it);
        }
    }

    void TextureManager::createCommonSamplers() {
        // Default sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_defaultSampler);

        // Cube sampler
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_cubeSampler);

        // Depth sampler
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.anisotropyEnable = VK_FALSE;
        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_depthSampler);

        // Register for cleanup
        VkDevice dev = m_device;
        VkSampler s1 = m_defaultSampler, s2 = m_cubeSampler, s3 = m_depthSampler;
        DeletionQueue::get().pushFunction("CommonSamplers", [dev, s1, s2, s3]() {
            vkDestroySampler(dev, s1, nullptr);
            vkDestroySampler(dev, s2, nullptr);
            vkDestroySampler(dev, s3, nullptr);
        });
    }

}
