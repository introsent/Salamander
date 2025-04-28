#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "command_pool_manager.h"

class CommandBuffer {
public:
    explicit CommandBuffer(std::shared_ptr<CommandPoolManager> poolManager, VkCommandBufferLevel level);
    ~CommandBuffer();

    VkCommandBuffer handle() const { return m_commandBuffer; }
    void begin();
    void end();
    void reset();

private:
    std::shared_ptr<CommandPoolManager> m_poolManager;
    VkCommandBuffer m_commandBuffer;
};