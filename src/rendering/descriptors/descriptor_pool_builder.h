// descriptor_pool_builder.h
#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class DescriptorPoolBuilder {
public:
    explicit DescriptorPoolBuilder(VkDevice device);

    DescriptorPoolBuilder& addPoolSize(VkDescriptorType type, uint32_t count);
    DescriptorPoolBuilder& setMaxSets(uint32_t count);
    DescriptorPoolBuilder& setFlags(VkDescriptorPoolCreateFlags flags);
    
    VkDescriptorPool build();

private:
    VkDevice m_device;
    std::vector<VkDescriptorPoolSize> m_poolSizes;
    uint32_t m_maxSets = 0;
    VkDescriptorPoolCreateFlags m_flags = 0;
};