// descriptor_set_layout.h
#pragma once
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
