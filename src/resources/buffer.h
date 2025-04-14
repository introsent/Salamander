#pragma once
#include "IBuffer.h"
#include "buffer_manager.h" 
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>

class Buffer : public IBuffer {
public:
    ManagedBuffer managedBuffer; 
    VmaAllocator allocator = VK_NULL_HANDLE; 

    Buffer(VmaAllocator alloc) : allocator(alloc) {}

    virtual VkBuffer getBuffer() const override {
        return managedBuffer.buffer;
    }

    virtual void destroy() override {
        if (managedBuffer.buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, managedBuffer.buffer, managedBuffer.allocation);
            managedBuffer.buffer = VK_NULL_HANDLE;
            managedBuffer.allocation = VK_NULL_HANDLE;
        }
    }

    virtual ~Buffer() {
        destroy();
    }
};
