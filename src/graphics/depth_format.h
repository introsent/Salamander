//
// Created by ivans on 14/04/2025.
//

#ifndef SALAMANDER_DEPTH_PIPELINE_H
#define SALAMANDER_DEPTH_PIPELINE_H


#include <vulkan/vulkan.h>
#include <vector>

namespace Salamander::Graphics {
    class DepthFormat {
    public:
        explicit DepthFormat(VkPhysicalDevice physicalDevice);

        [[nodiscard]] VkFormat handle() const
        {
            return m_depthFormat;
        }

    private:
        [[nodiscard]] VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                                     VkImageTiling tiling,
                                     VkFormatFeatureFlags features) const;

        [[nodiscard]] VkFormat findDepthFormat() const;

        VkPhysicalDevice m_physicalDevice;
        VkFormat m_depthFormat;
    };
}


#endif //SALAMANDER_DEPTH_PIPELINE_H
