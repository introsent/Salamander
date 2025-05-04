// descriptor_set_layout_builder.cpp
#include "descriptor_set_layout_builder.h"
#include <stdexcept>

DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(VkDevice device) 
    : m_device(device) {}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addBinding(
    uint32_t binding,
    VkDescriptorType type,
    VkShaderStageFlags stageFlags,
    uint32_t descriptorCount
) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = descriptorCount;
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = nullptr;

    m_bindings.push_back(layoutBinding);
    return *this;
}

std::unique_ptr<DescriptorSetLayout> DescriptorSetLayoutBuilder::build() {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());
    layoutInfo.pBindings = m_bindings.data();

    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

    return std::make_unique<DescriptorSetLayout>(m_device, layout);
}