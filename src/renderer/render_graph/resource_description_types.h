//
// Created by ivans on 28/03/2026.
//

#ifndef SALAMANDER_RESOURCE_DESCRIPTION_TYPES_H
#define SALAMANDER_RESOURCE_DESCRIPTION_TYPES_H
#include <vulkan/vulkan.h>

namespace Salamander::Renderer::RenderGraph
{
    enum class SizePolicy {
        Fixed, // dimensions are known upfront
        SwapchainRelative, // dimensions should match or be fraction of the swapchain
        Relative // dimensions are relative to other resource
    };

    enum class LifetimeInfo {
        Persistent, // resource needs to survive till the next frame
        Transient // resource only needs to exist during this frame
    };

    struct ImageAttachmentDescription {
        SizePolicy sizePolicy = SizePolicy::SwapchainRelative;
        float width = 1.f;
        float height = 1.f;
        VkFormat format = VK_FORMAT_UNDEFINED;
        int samples = 1;
        int levels = 1;
        int layers = 1;
        LifetimeInfo lifetimeInfo = LifetimeInfo::Transient;

        bool operator==(const ImageAttachmentDescription &) const;
    };

    struct BufferAttachmentDescription {
        VkDeviceSize size = 0;
        VkBufferUsageFlags usage = 0;
        LifetimeInfo lifetimeInfo = LifetimeInfo::Transient;

        bool operator==(const BufferAttachmentDescription &) const;
    };
}
#endif //SALAMANDER_RESOURCE_DESCRIPTION_TYPES_H