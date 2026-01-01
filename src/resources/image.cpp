//
// Created by ivans on 05/12/2025.
//

#include "image.h"
#include "deletion_queue.h"

static int imageID = 0;

Image::Image(VmaAllocator allocator) : m_allocator(allocator)
{
}

void Image::create(const uint32_t width, const uint32_t height,
                   const VkFormat format,
                   const VkImageTiling tiling,
                   const VkImageUsageFlags usage,
                   const VmaMemoryUsage memoryUsage,
                   const uint32_t layers,
                   const VkImageCreateFlags flags,
                   const uint32_t mipLevels)
{
    // fill class data members
    m_width = width;
    m_height = height;
    m_mipLevels = mipLevels;
    m_format = format;
    m_usage = usage;
    m_tiling = tiling;
    m_layers = layers;
    m_flags = flags;

    // determine aspect based on format
    if (m_format == VK_FORMAT_D32_SFLOAT ||
    m_format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
    m_format == VK_FORMAT_D24_UNORM_S8_UINT)
    {
        m_aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        // + stencil, if needed
    }
    else {
        m_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // fill VkImageCreateInfo
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = m_flags;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {  m_width, m_height, 1 };
    imageInfo.mipLevels = m_mipLevels;
    imageInfo.arrayLayers =  m_layers;
    imageInfo.format = m_format;
    imageInfo.tiling = m_tiling;
    imageInfo.initialLayout = m_currentLayout;
    imageInfo.usage = m_usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // create image using vma allocation
    m_memoryUsage = memoryUsage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage =  m_memoryUsage;

    if (vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan image!");
    }

    // add to deletion queue
    VmaAllocator allocCopy = m_allocator;
    VkImage imageCopy = m_image;
    VmaAllocation allocHandle = m_allocation;

    DeletionQueue::get().pushFunction("Image_" + std::to_string(imageID++),
        [allocCopy, imageCopy, allocHandle]() {
            vmaDestroyImage(allocCopy, imageCopy, allocHandle);
        });
}

void Image::transitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout)
{
    transitionLayoutEx(
        cmd,
        m_currentLayout,
        newLayout,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
        VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT
    );

    m_currentLayout = newLayout;
}

void Image::transitionLayoutEx(
    VkCommandBuffer cmd,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags2 srcStage,
    VkPipelineStageFlags2 dstStage,
    VkAccessFlags2 srcAccess,
    VkAccessFlags2 dstAccess,
    uint32_t baseMip,
    uint32_t mipCount,
    uint32_t baseLayer,
    uint32_t layerCount
)
{
    VkImageMemoryBarrier2 barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2
    };
    barrier.srcStageMask = srcStage;
    barrier.dstStageMask = dstStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;

    barrier.subresourceRange = {
        m_aspect,
        baseMip,
        mipCount,
        baseLayer,
        layerCount
    };

    VkDependencyInfo depInfo{
        VK_STRUCTURE_TYPE_DEPENDENCY_INFO
    };
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);

    m_currentLayout = newLayout;
}

void Image::copyFromBuffer(VkCommandBuffer cmd, VkBuffer buffer) const
{
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { m_width, m_height, 1 };

    vkCmdCopyBufferToImage(cmd, buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}



