#include "command_manager.h"
#include <stdexcept>

CommandManager::CommandManager(VkDevice device, uint32_t queueFamilyIndex, VkQueue graphicsQueue)
    : m_device(device), m_graphicsQueue(graphicsQueue) {
    // Initialize the CommandPoolManager with the device and queue family index
    m_commandPoolManager = std::make_unique<CommandPoolManager>(device, queueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
}

CommandManager::~CommandManager() = default;

VkCommandBuffer CommandManager::beginSingleTimeCommands() const {
    // Allocate a command buffer for one-time use
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPoolManager->handle();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate one-time command buffer!");
    }

    // Begin recording the command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording one-time command buffer!");
    }

    return commandBuffer;
}

void CommandManager::endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
    // End recording the command buffer
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record one-time command buffer!");
    }

    // Submit the command buffer to the graphics queue
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit one-time command buffer!");
    }

    // Wait for the queue to finish execution
    vkQueueWaitIdle(m_graphicsQueue);

    // Free the command buffer
    vkFreeCommandBuffers(m_device, m_commandPoolManager->handle(), 1, &commandBuffer);
}

std::unique_ptr<CommandBuffer> CommandManager::createCommandBuffer(VkCommandBufferLevel level) const {
    // Delegate command buffer allocation to the CommandPoolManager
    return m_commandPoolManager->allocateCommandBuffer(level);
}

void CommandManager::recordCommandBuffer(CommandBuffer& commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extent) const
{
    // Delegate command buffer recording to the CommandPoolManager
    m_commandPoolManager->recordCommandBuffer(commandBuffer, renderPass, framebuffer, extent);
}
