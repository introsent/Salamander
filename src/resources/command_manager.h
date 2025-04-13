#pragma once

#include <vulkan/vulkan.h>

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

    

private:
    VkDevice m_device;
    VkCommandPool m_commandPool;
    VkQueue m_graphicsQueue;
};
