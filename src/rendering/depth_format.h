#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class DepthFormat {
public:
    DepthFormat(VkPhysicalDevice physicalDevice);

    VkFormat handle() const { return m_depthFormat; }

private:
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features) const;

    VkFormat findDepthFormat() const;

    VkPhysicalDevice m_physicalDevice;
    VkFormat m_depthFormat;
};
