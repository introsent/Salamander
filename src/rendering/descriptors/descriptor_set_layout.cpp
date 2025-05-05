// descriptor_set_layout.cpp
#include "descriptor_set_layout.h"
#include "deletion_queue.h"

DescriptorSetLayout::DescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout)
    : m_device(device)
    , m_layout(layout)
{
    DeletionQueue::get().pushFunction("DescriptorSetLayout", [device = m_device, layout = m_layout]() {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    });
}

DescriptorSetLayout::~DescriptorSetLayout() = default;