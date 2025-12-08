#include "texture_manager.h"
#include "buffer_manager.h"
#include "command_manager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdexcept>
#include "deletion_queue.h"

TextureManager::TextureManager(VkDevice device, VmaAllocator allocator, CommandManager* cmdManager,
    BufferManager* bufferManager, DebugMessenger* debugMessenger) :
    m_device(device), m_allocator(allocator), m_commandManager(cmdManager), m_bufferManager(bufferManager),
    m_debugMessenger(debugMessenger)
{

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
    ManagedBuffer staging = m_bufferManager->createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    void* data;
    vmaMapMemory(m_allocator, staging.allocation, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(m_allocator, staging.allocation);
    stbi_image_free(pixels);

    uint32_t mipsLevels = 1;
    if (generateMips)
    {
        mipsLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    }


    // Create the GPU image with the supplied format
    auto image = std::make_unique<Image>(m_allocator);
    image->create(texWidth, texHeight,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, 1, 0, mipsLevels);

    // Transition, copy, and transition again
    VkCommandBuffer cmd = m_commandManager->beginSingleTimeCommands();
    image->transitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    image->copyFromBuffer(cmd, staging.buffer);
    image->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    m_commandManager->endSingleTimeCommands(cmd);

    // Create texture based on image
    auto texture = std::make_unique<Texture>(m_device);
    texture->create(std::move(image)); // image view and sampler creation already in create
    texture->createImageView();
    texture->createSampler();

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

    ManagedBuffer staging = m_bufferManager->createBuffer(
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
    texture->createSampler();

    m_textures.insert({path, std::move(texture)});
    return *m_textures.at(path);
}

Texture& TextureManager::createTexture(
    uint32_t width, uint32_t height,
    VkFormat format,
    VkImageUsageFlags usage,
    VmaMemoryUsage memoryUsage,
    VkImageAspectFlags aspect,
    bool generateMipMap,
    bool createSampler,
    const std::string& debugName)
{
    auto image = std::make_unique<Image>(m_allocator);
    image->create(width, height, format,
                  VK_IMAGE_TILING_OPTIMAL,
                  usage, memoryUsage,
                  1, 0, 1);

    auto texture = std::make_unique<Texture>(m_device);
    texture->create(std::move(image));
    texture->createImageView();
    if (createSampler)
    {
        texture->createSampler();
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

    ManagedBuffer staging = m_bufferManager->createBuffer(
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
    texture->createSampler();

    if (!debugName.empty()) {
        texture->setDebugName(m_debugMessenger, debugName);
    }

    m_textures.insert({debugName, std::move(texture)});
    return *m_textures.at(debugName);
}

Texture& TextureManager::createCubeTexture(uint32_t size, VkFormat format,
                                                VkImageUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    auto image = std::make_unique<Image>(m_allocator);
    image->create(size, size, format,
                  VK_IMAGE_TILING_OPTIMAL,
                  usage,
                  memoryUsage,
                  1, // mipLevels
                  0, // flags if needed
                  6  // layers for cubemap
    );

    auto texture = std::make_unique<Texture>(m_device);
    texture->create(std::move(image));
    texture->createCubeImageView();
    texture->createCubeSampler();

    texture->setDebugName(m_debugMessenger, "CubeMap");

    std::string key = "CubeTexture_" + std::to_string(m_textures.size());
    m_textures.insert({ key, std::move(texture) });

    return *m_textures.at(key);
}