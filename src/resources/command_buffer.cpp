#include "command_buffer.h"

#include <iostream>

#include "command_pool_manager.h" 
#include "../deletion_queue.h"
#include <stdexcept>
#include <string>

CommandBuffer::CommandBuffer(
    std::shared_ptr<CommandPoolManager> poolManager,
    VkCommandBufferLevel level
) : m_poolManager(poolManager) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_poolManager->handle();
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    // Validate the command pool handle
    if (allocInfo.commandPool == VK_NULL_HANDLE) {
        throw std::runtime_error("Command pool is null during buffer allocation!");
    }

    VkResult result = vkAllocateCommandBuffers(
        m_poolManager->device(),
        &allocInfo,
        &m_commandBuffer
    );

    if (result != VK_SUCCESS || m_commandBuffer == VK_NULL_HANDLE) {
        throw std::runtime_error("Command buffer allocation failed! Error: " + std::to_string(result));
    }
}

CommandBuffer::~CommandBuffer() {
    VkDevice device = m_poolManager->device();
    VkCommandPool pool = m_poolManager->handle();
    VkCommandBuffer buffer = m_commandBuffer;
    DeletionQueue::get().pushFunction([device, pool, buffer]() {
        vkFreeCommandBuffers(device, pool, 1, &buffer);
        });
}

void CommandBuffer::begin() {
    if (m_commandBuffer == VK_NULL_HANDLE) {
        throw std::runtime_error("Command buffer is null!");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(m_commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer!");
    }
}

void CommandBuffer::end() {
    if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer!");
    }
}

void CommandBuffer::reset()
{
    vkResetCommandBuffer(m_commandBuffer, 0); // Or VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT
}
