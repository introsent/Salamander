// descriptor_manager_base.h
#pragma once
#include <vulkan/vulkan.h>

class DescriptorManagerBase {
public:
    virtual ~DescriptorManagerBase() = default;
    virtual VkDescriptorPool getPool() const = 0;
protected:
    VkDevice m_device{};
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
};