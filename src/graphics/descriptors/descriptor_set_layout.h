//
// Created by ivans on 04/05/2025.
//

#ifndef SALAMANDER_DESCRIPTOR_SET_LAYOUT_H
#define SALAMANDER_DESCRIPTOR_SET_LAYOUT_H


#include <vulkan/vulkan.h>

namespace Salamander::Graphics::Descriptors {
    class DescriptorSetLayout final {
    public:
        DescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout);
        ~DescriptorSetLayout();

        DescriptorSetLayout(const DescriptorSetLayout &) = delete;
        DescriptorSetLayout & operator=(const DescriptorSetLayout &) = delete;
        DescriptorSetLayout(DescriptorSetLayout &&) = delete;
        DescriptorSetLayout & operator=(DescriptorSetLayout &&) = delete;

        [[nodiscard]] VkDescriptorSetLayout handle() const { return m_layout;}

    private:
        VkDevice m_device;
        VkDescriptorSetLayout m_layout;
    };
}


#endif //SALAMANDER_DESCRIPTOR_SET_LAYOUT_H