#include "command_pool_manager.h"

#include <iostream>
#include <ostream>
#include <stdexcept>

#include "command_buffer.h"
#include "../deletion_queue.h"

CommandPoolManager::CommandPoolManager(
    VkDevice device,
    uint32_t queueFamilyIndex,
    VkCommandPoolCreateFlags flags
) : m_device(device) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = flags;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

std::shared_ptr<CommandPoolManager> CommandPoolManager::create(
    VkDevice device,
    uint32_t queueFamilyIndex,
    VkCommandPoolCreateFlags flags
) {
    return std::shared_ptr<CommandPoolManager>(
        new CommandPoolManager(device, queueFamilyIndex, flags)
    );
}

std::unique_ptr<CommandBuffer> CommandPoolManager::allocateCommandBuffer(VkCommandBufferLevel level) {
    return std::make_unique<CommandBuffer>(shared_from_this(), level);
}