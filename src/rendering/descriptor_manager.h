#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

class DescriptorManager {
public:
    DescriptorManager(
        VkDevice device,
        VkDescriptorSetLayout layout,
        const std::vector<VkDescriptorPoolSize>& poolSizes,
        uint32_t maxSets
    );
    DescriptorManager(const DescriptorManager&) = delete;
    DescriptorManager& operator=(const DescriptorManager&) = delete;
    DescriptorManager(DescriptorManager&&) = delete;
    DescriptorManager& operator=(DescriptorManager&&) = delete;

    void updateDescriptorSet(
        uint32_t setIndex,
        VkDescriptorBufferInfo bufferInfo,
        VkDescriptorImageInfo imageInfo
    ) const;

    const std::vector<VkDescriptorSet>& getDescriptorSets() const { return m_descriptorSets; }
    VkDescriptorPool getPool() const { return m_descriptorPool; }

private:
    VkDevice m_device;
    VkDescriptorPool m_descriptorPool;
    VkDescriptorSetLayout m_layout;
    std::vector<VkDescriptorSet> m_descriptorSets;
};