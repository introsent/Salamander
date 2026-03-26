//
// Created by ivans on 04/05/2025.
//

#ifndef SALAMANDER_DESCRIPTOR_POOL_BUILDER_H
#define SALAMANDER_DESCRIPTOR_POOL_BUILDER_H

#include <vulkan/vulkan.h>
#include <vector>

namespace Salamander::Graphics::Descriptors {
    class DescriptorPoolBuilder {
        public
        :
        explicit DescriptorPoolBuilder (VkDevice device);

        DescriptorPoolBuilder & addPoolSize(VkDescriptorType type, uint32_t count);
        DescriptorPoolBuilder & setMaxSets(uint32_t count);
        DescriptorPoolBuilder & setFlags(VkDescriptorPoolCreateFlags flags);

        VkDescriptorPool build();

        private
        :
        VkDevice m_device;
        std::vector<VkDescriptorPoolSize> m_poolSizes;
        uint32_t m_maxSets = 0;
        VkDescriptorPoolCreateFlags m_flags = 0;
    };
}


#endif //SALAMANDER_DESCRIPTOR_POOL_BUILDER_H