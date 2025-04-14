#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>

class CommandManager;
class CommandBuffer {
public:
    // Constructor for CommandManager-based initialization
    CommandBuffer(CommandManager* manager, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // New constructor for direct initialization with VkDevice and VkCommandPool
    CommandBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBufferLevel level);
    ~CommandBuffer();

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    CommandBuffer(CommandBuffer&&) = delete;
    CommandBuffer& operator=(CommandBuffer&&) = delete;

    void begin(VkCommandBufferUsageFlags usageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) const;

    void end() const;

    void submit(VkQueue queue, VkFence fence = VK_NULL_HANDLE) const;

    void reset() const;

    // Access the raw Vulkan command buffer
    VkCommandBuffer handle() const { return m_commandBuffer; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    CommandManager* m_manager = nullptr;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
};
