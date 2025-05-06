#include "buffer_manager.h"
#include <stdexcept>
#include "command_manager.h"
#include "deletion_queue.h"

BufferManager::BufferManager(VkDevice device, VmaAllocator allocator, CommandManager* commandManager)
    : m_device(device), m_allocator(allocator), m_commandManager(commandManager) {
}

ManagedBuffer BufferManager::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    ManagedBuffer managedBuffer{};
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;

    if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &managedBuffer.buffer, &managedBuffer.allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    ManagedBuffer localBuf = managedBuffer;  
    VmaAllocator  alloc = m_allocator;       

    static int bufferID = 0;
    DeletionQueue::get().pushFunction("Buffer_" + std::to_string(bufferID++), [alloc, localBuf]() {
        vmaDestroyBuffer(alloc,
            localBuf.buffer,
            localBuf.allocation);
        });

    // Store the created buffer for later cleanup.
    m_managedBuffers.push_back(managedBuffer);
    return managedBuffer;
}

void BufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const {
    VkCommandBuffer cmd = m_commandManager->beginSingleTimeCommands();

    // 1. Copy operation
    VkBufferCopy copyRegion{ .size = size };
    vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

    // 2. Critical barrier for SSBO/vertex access
    VkBufferMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = dstBuffer,
        .offset = 0,
        .size = size
    };
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        0, 0, nullptr, 1, &barrier, 0, nullptr
    );

    m_commandManager->endSingleTimeCommands(cmd);
}
