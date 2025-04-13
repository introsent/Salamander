#include "buffer_manager.h"
#include <stdexcept>
#include "command_manager.h"

BufferManager::BufferManager(VkDevice device, VmaAllocator allocator, CommandManager* commandManager)
    : m_device(device), m_allocator(allocator), m_commandManager(commandManager) {
}

void BufferManager::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage, VkBuffer& buffer,
    VmaAllocation& allocation) const {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;

    if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
}

void BufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
{
    VkCommandBuffer commandBuffer = m_commandManager->beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    m_commandManager->endSingleTimeCommands(commandBuffer);
}
