// descriptor_set_layout_builder.h
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "descriptor_set_layout.h"

class DescriptorSetLayoutBuilder {
public:
    explicit DescriptorSetLayoutBuilder(VkDevice device);

    DescriptorSetLayoutBuilder& addBinding(
        uint32_t binding,
        VkDescriptorType type,
        VkShaderStageFlags stageFlags,
        uint32_t descriptorCount = 1
    );

    std::unique_ptr<DescriptorSetLayout> build();

private:
    VkDevice m_device;
    std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};