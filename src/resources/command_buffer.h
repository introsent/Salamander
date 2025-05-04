#pragma once
#include <vulkan/vulkan.h>

class CommandBuffer {
public:
    CommandBuffer(VkCommandBuffer handle, VkCommandPool pool);

    void begin(VkCommandBufferUsageFlags flags = 0);
    void end();
    void reset();

    VkCommandBuffer handle() const { return m_handle; }

private:
    VkCommandBuffer m_handle;
    VkCommandPool m_pool;
};