#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "command_pool_manager.h"
#include "command_buffer.h"

class CommandManager {
public:
    CommandManager(VkDevice device, uint32_t queueFamilyIndex, VkQueue graphicsQueue);
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

    // Record a command buffer
    void recordCommandBuffer(CommandBuffer& commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extent) const;

    // Accessors
    VkQueue graphicsQueue() const { return m_graphicsQueue; }

private:
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    std::unique_ptr<CommandPoolManager> m_commandPoolManager;
};
