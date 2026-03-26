//
// Created by ivans on 04/05/2025.
//

#ifndef SALAMANDER_DESCRIPTOR_SET_LAYOUT_BUILDER_H
#define SALAMANDER_DESCRIPTOR_SET_LAYOUT_BUILDER_H


#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "descriptor_set_layout.h"

namespace Salamander::Graphics::Descriptors {
    class DescriptorSetLayoutBuilder {
    public:
        explicit DescriptorSetLayoutBuilder (VkDevice device);

        DescriptorSetLayoutBuilder & addBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags,
            uint32_t descriptorCount = 1);

        std::unique_ptr<DescriptorSetLayout> build();

    private:
        VkDevice m_device;
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;
    };
}


#endif //SALAMANDER_DESCRIPTOR_SET_LAYOUT_BUILDER_H