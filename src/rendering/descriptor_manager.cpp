#include "descriptor_manager.h"
#include <array>

#include "../deletion_queue.h"

DescriptorManager::DescriptorManager(
    VkDevice device,
    VkDescriptorSetLayout layout,
    const std::vector<VkDescriptorPoolSize>& poolSizes,
    uint32_t maxSets
) : m_device(device), m_layout(layout) {
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    VkDevice         dev = m_device;
    VkDescriptorPool pool = m_descriptorPool;

    DeletionQueue::get().pushFunction("Descriptor pool", [dev, pool]() {
        vkDestroyDescriptorPool(dev, pool, nullptr);
        });

    std::vector<VkDescriptorSetLayout> layouts(maxSets, m_layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = maxSets;
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(maxSets);
    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }
}

void DescriptorManager::updateDescriptorSet(
    uint32_t setIndex,
    VkDescriptorBufferInfo bufferInfo,
    VkDescriptorImageInfo imageInfo
) const
{
    std::array<VkWriteDescriptorSet, 2> writes{};

    // Uniform buffer write
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = m_descriptorSets[setIndex];
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo = &bufferInfo;

    // Combined image sampler write
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_descriptorSets[setIndex];
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
