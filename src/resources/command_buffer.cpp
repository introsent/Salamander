#include "command_buffer.h"
#include "command_manager.h"

CommandBuffer::CommandBuffer(CommandManager* manager, VkCommandBufferLevel level)
    : m_manager(manager), m_device(manager->device()), m_commandBuffer(VK_NULL_HANDLE) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = manager->commandPool();
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(manager->device(), &allocInfo, &m_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer!");
    }
}

CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBufferLevel level)
    : m_manager(nullptr), m_device(device), m_commandBuffer(VK_NULL_HANDLE) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &m_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer!");
    }
}

CommandBuffer::~CommandBuffer() {
    if (m_commandBuffer != VK_NULL_HANDLE) {
        VkCommandPool commandPool = m_manager ? m_manager->commandPool() : VK_NULL_HANDLE;
        if (m_device != VK_NULL_HANDLE && commandPool != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(m_device, commandPool, 1, &m_commandBuffer);
        }
        m_commandBuffer = VK_NULL_HANDLE; // Prevent double-free
    }
}
void CommandBuffer::begin(VkCommandBufferUsageFlags usageFlags) const {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = usageFlags;

    if (vkBeginCommandBuffer(m_commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }
}

void CommandBuffer::end() const
{
    if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void CommandBuffer::submit(VkQueue queue, VkFence fence) const
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit command buffer!");
    }

    vkQueueWaitIdle(queue); // Optional: Wait for execution to complete
}

void CommandBuffer::reset() const
{
    if (vkResetCommandBuffer(m_commandBuffer, 0) != VK_SUCCESS) {
        throw std::runtime_error("Failed to reset command buffer!");
    }
}
