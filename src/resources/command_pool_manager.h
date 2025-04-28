#pragma once
#include <vulkan/vulkan.h>
#include <memory>

class CommandBuffer;
class CommandPoolManager : public std::enable_shared_from_this<CommandPoolManager> {
public:
    static std::shared_ptr<CommandPoolManager> create(
        VkDevice device,
        uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags = 0
    );

    VkDevice device() const { return m_device; } 
    std::unique_ptr<CommandBuffer> allocateCommandBuffer(VkCommandBufferLevel level);
    VkCommandPool handle() const { return m_commandPool; }

private:
    CommandPoolManager(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);
    VkDevice m_device;
    VkCommandPool m_commandPool;
};