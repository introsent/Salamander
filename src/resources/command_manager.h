#pragma once
#include <memory>
#include "command_buffer.h"
class CommandManager {
public:
    CommandManager(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue);
    ~CommandManager();
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;
    CommandManager(CommandManager&&) = delete;
    CommandManager& operator=(CommandManager&&) = delete;

    // Begins a one-time command buffer
    VkCommandBuffer beginSingleTimeCommands() const;

    // Ends and submits a one-time command buffer
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    // Create a reusable CommandBuffer
    std::unique_ptr<CommandBuffer> createCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    // Accessors
    VkDevice device() const { return m_device; }
    VkCommandPool commandPool() const { return m_commandPool; }
    VkQueue graphicsQueue() const { return m_graphicsQueue; }

private:
    VkDevice m_device;
    VkCommandPool m_commandPool;
    VkQueue m_graphicsQueue;
};
