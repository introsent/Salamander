//
// Created by ivans on 05/12/2025.
//

#ifndef SALAMANDER_IMAGE_H
#define SALAMANDER_IMAGE_H
#include "application.h"
#include "image_transition_manager.h"

namespace Salamander::Resources::Textures
{
    class Image
    {
    public:
        explicit Image(VmaAllocator allocator);

        void create(uint32_t width, uint32_t height,
                    VkFormat format,
                    VkImageTiling tiling,
                    VkImageUsageFlags usage,
                    VmaMemoryUsage memoryUsage,
                    uint32_t layers = 1,
                    VkImageCreateFlags flags = 0,
                    uint32_t mipLevels = 1);

        // simple transition
        void transitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout);

        // full-control transition
        void transitionLayoutEx(
            VkCommandBuffer cmd,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            VkPipelineStageFlags2 srcStage,
            VkPipelineStageFlags2 dstStage,
            VkAccessFlags2 srcAccess,
            VkAccessFlags2 dstAccess,
            uint32_t baseMip = 0,
            uint32_t mipCount = VK_REMAINING_MIP_LEVELS,
            uint32_t baseLayer = 0,
            uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS
        ) ;

        void copyFromBuffer(VkCommandBuffer cmd, VkBuffer buffer) const;

        void generateMipmaps(VkCommandBuffer cmd);

        [[nodiscard]] VkImage handle() const { return m_image; }
        [[nodiscard]] glm::ivec2 size() const { return {m_width, m_height}; }
        [[nodiscard]] VkFormat format() const { return m_format; }
        [[nodiscard]] uint32_t mipLevels() const { return m_mipLevels; }
        [[nodiscard]] VkImageTiling tiling() const { return m_tiling; }
        [[nodiscard]] VkImageUsageFlags usage() const { return m_usage; }
        [[nodiscard]] VkImageCreateFlags flags() const { return m_flags; }
        [[nodiscard]] VkImageAspectFlags aspect() const { return m_aspect; }
        [[nodiscard]] uint32_t layers() const { return m_layers; }

        [[nodiscard]] VkImageLayout currentLayout() const { return m_currentLayout; }

    private:
        VmaAllocator m_allocator;

        VkImage m_image = VK_NULL_HANDLE;

        VmaAllocation m_allocation = nullptr;
        VmaMemoryUsage m_memoryUsage = VMA_MEMORY_USAGE_UNKNOWN;

        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_mipLevels = 0;
        VkFormat m_format = VK_FORMAT_UNDEFINED;
        VkImageUsageFlags m_usage = 0;
        VkImageAspectFlags m_aspect = 0;
        VkImageTiling m_tiling = VK_IMAGE_TILING_LINEAR;
        VkImageCreateFlags m_flags = 0;
        uint32_t m_layers = 0;

        VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };
}



#endif //SALAMANDER_IMAGE_H