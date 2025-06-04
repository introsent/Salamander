#include "depth_format.h"
#include <stdexcept>

DepthFormat::DepthFormat(VkPhysicalDevice physicalDevice)
    : m_physicalDevice(physicalDevice) {
    m_depthFormat = findDepthFormat();
}

// Finds a supported format from the given candidates based on the specified tiling and features
VkFormat DepthFormat::findSupportedFormat(const std::vector<VkFormat>& candidates,
    const VkImageTiling tiling,
    VkFormatFeatureFlags features) const {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

// Finds a suitable depth format from the available formats
VkFormat DepthFormat::findDepthFormat() const {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}
