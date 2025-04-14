#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>

class CommandManager; 
class CommandBuffer {
public:
    CommandBuffer(CommandManager* manager, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    ~CommandBuffer();
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    CommandBuffer(CommandBuffer&&) = delete;
    CommandBuffer& operator=(CommandBuffer&&) = delete;

    // Begin recording commands
    void begin(VkCommandBufferUsageFlags usageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) const;

    // End recording commands
    void end() const;

    // Submit the command buffer for execution
    void submit(VkQueue queue, VkFence fence = VK_NULL_HANDLE) const;

    // Access the raw Vulkan command buffer
    VkCommandBuffer handle() const { return m_commandBuffer; }

private:
    CommandManager* m_manager;
    VkCommandBuffer m_commandBuffer;
};