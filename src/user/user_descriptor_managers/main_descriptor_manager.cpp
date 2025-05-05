// descriptor_manager.cpp
#include "main_descriptor_manager.h"
#include "deletion_queue.h"
#include <stdexcept>

MainDescriptorManager::MainDescriptorManager(
    VkDevice device,
    VkDescriptorSetLayout layout,
    const std::vector<VkDescriptorPoolSize>& poolSizes,
    uint32_t maxSets
) : m_layout(layout) {
    m_device = device;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    DeletionQueue::get().pushFunction("DescriptorPool", [device, pool = m_pool]() {
        vkDestroyDescriptorPool(device, pool, nullptr);
    });

    std::vector<VkDescriptorSetLayout> layouts(maxSets, m_layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = maxSets;
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(maxSets);
    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }
}

void MainDescriptorManager::updateDescriptorSet(
    uint32_t setIndex,
    const std::vector<DescriptorUpdateInfo>& updateInfo
) const {
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(updateInfo.size());

    for (const auto& info : updateInfo) {
        VkWriteDescriptorSet writeSet{};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = m_descriptorSets[setIndex];
        writeSet.dstBinding = info.binding;
        writeSet.descriptorType = info.type;
        writeSet.descriptorCount = 1;
        
        if (info.isImage) {
            writeSet.pImageInfo = &info.imageInfo;
        } else {
            writeSet.pBufferInfo = &info.bufferInfo;
        }

        descriptorWrites.push_back(writeSet);
    }

    vkUpdateDescriptorSets(
        m_device,
        static_cast<uint32_t>(descriptorWrites.size()),
        descriptorWrites.data(),
        0,
        nullptr
    );
}