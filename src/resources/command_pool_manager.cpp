#include "command_pool_manager.h"

#include <array>

CommandPoolManager::CommandPoolManager(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
    : m_device(device) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = flags;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

CommandPoolManager::~CommandPoolManager() {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}

std::unique_ptr<CommandBuffer> CommandPoolManager::allocateCommandBuffer(VkCommandBufferLevel level) {
    return std::make_unique<CommandBuffer>(m_device, m_commandPool, level);
}
