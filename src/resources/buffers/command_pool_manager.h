//
// Created by ivans on 14/04/2025.
//

#ifndef SALAMANDER_COMMAND_POOL_MANAGER_H
#define SALAMANDER_COMMAND_POOL_MANAGER_H


#include <vulkan/vulkan.h>
#include <memory>

namespace Salamander::Resources::Buffers {
    class CommandBuffer;

    class CommandPoolManager : public std::enable_shared_from_this<CommandPoolManager> {
    public:
        static std::shared_ptr<CommandPoolManager> create(
            VkDevice device,
            uint32_t queueFamilyIndex,
            VkCommandPoolCreateFlags flags = 0
        );

        VkDevice device() const { return m_device; }

        std::unique_ptr<CommandBuffer> allocateCommandBuffer(VkCommandBufferLevel level);

        VkCommandPool handle() const { return m_commandPool; }

    private:
        CommandPoolManager(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);

        VkDevice m_device;
        VkCommandPool m_commandPool{};
    };
}


#endif //SALAMANDER_COMMAND_POOL_MANAGER_H
