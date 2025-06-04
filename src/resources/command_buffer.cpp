#include "command_buffer.h"
#include <stdexcept>

CommandBuffer::CommandBuffer(VkCommandBuffer handle, VkCommandPool pool)
    : m_handle(handle), m_pool(pool) {
}

void CommandBuffer::begin(VkCommandBufferUsageFlags flags) const {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;

    if (vkBeginCommandBuffer(m_handle, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer!");
    }
}

void CommandBuffer::end() const {
    if (vkEndCommandBuffer(m_handle) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer!");
    }
}

void CommandBuffer::reset() const {
    vkResetCommandBuffer(m_handle, 0);
}
