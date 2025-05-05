#include "descriptor_pool_builder.h"
#include "deletion_queue.h"
#include <stdexcept>

DescriptorPoolBuilder::DescriptorPoolBuilder(VkDevice device) 
    : m_device(device) {}

DescriptorPoolBuilder& DescriptorPoolBuilder::addPoolSize(VkDescriptorType type, uint32_t count) {
    m_poolSizes.push_back({type, count});
    return *this;
}

DescriptorPoolBuilder& DescriptorPoolBuilder::setMaxSets(uint32_t count) {
    m_maxSets = count;
    return *this;
}

DescriptorPoolBuilder& DescriptorPoolBuilder::setFlags(VkDescriptorPoolCreateFlags flags) {
    m_flags = flags;
    return *this;
}

VkDescriptorPool DescriptorPoolBuilder::build() {
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = m_flags;
    poolInfo.maxSets = m_maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
    poolInfo.pPoolSizes = m_poolSizes.data();

    VkDescriptorPool pool;
    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }

    return pool;
}