#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

class CommandManager;

struct ManagedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

class BufferManager {
public:
    BufferManager(VkDevice device, VmaAllocator allocator, CommandManager* commandManager);
    ~BufferManager();
    BufferManager(const BufferManager&) = delete;
    BufferManager& operator=(const BufferManager&) = delete;
    BufferManager(BufferManager&&) = delete;
    BufferManager& operator=(BufferManager&&) = delete;

    ManagedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
   
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    

private:
    void cleanup();

    VkDevice m_device;
    VmaAllocator m_allocator;
    CommandManager* m_commandManager;
    std::vector<ManagedBuffer> m_managedBuffers;
};
