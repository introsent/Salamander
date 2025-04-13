#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

class CommandManager;

class BufferManager {
public:
    BufferManager(VkDevice device, VmaAllocator allocator, CommandManager* commandManager);
    ~BufferManager() = default;
    BufferManager(const BufferManager&) = delete;
    BufferManager& operator=(const BufferManager&) = delete;
    BufferManager(BufferManager&&) = delete;
    BufferManager& operator=(BufferManager&&) = delete;

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
        VkBuffer& buffer, VmaAllocation& allocation) const;

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    

private:
    VkDevice m_device;
    VmaAllocator m_allocator;
    CommandManager* m_commandManager;
};
