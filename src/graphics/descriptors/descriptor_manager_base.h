//
// Created by ivans on 04/05/2025.
//

#ifndef SALAMANDER_DESCRIPTOR_MANAGER_BASE_H
#define SALAMANDER_DESCRIPTOR_MANAGER_BASE_H


#include <vulkan/vulkan.h>

namespace Salamander::Graphics::Descriptors {
    class DescriptorManagerBase {
    public:
        virtual ~DescriptorManagerBase() = default;
        [[nodiscard]] virtual VkDescriptorPool getPool() const = 0;

    protected:
        VkDevice m_device{};
        VkDescriptorPool m_pool = VK_NULL_HANDLE;
    };
}


#endif //SALAMANDER_DESCRIPTOR_MANAGER_BASE_H