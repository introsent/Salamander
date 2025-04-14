#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include "command_buffer.h"
#include <memory>

class CommandPoolManager {
public:
    CommandPoolManager(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    ~CommandPoolManager();

    // Allocate a command buffer
    std::unique_ptr<CommandBuffer> allocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // Access the raw Vulkan command pool
    VkCommandPool handle() const { return m_commandPool; }

private:
    VkDevice m_device;
    VkCommandPool m_commandPool;
};
