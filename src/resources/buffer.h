#pragma once
#include "ibuffer.h"
#include "buffer_manager.h"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>

class Buffer : public IBuffer {
public:
    ManagedBuffer managedBuffer;
    VmaAllocator allocator = VK_NULL_HANDLE;

    Buffer() = default;
    Buffer(VmaAllocator alloc) : allocator(alloc) {}

    // Move Constructor
    Buffer(Buffer&& other) noexcept {
        managedBuffer = other.managedBuffer;
        allocator = other.allocator;
        other.managedBuffer = { VK_NULL_HANDLE, VK_NULL_HANDLE }; // Invalidate source
        other.allocator = VK_NULL_HANDLE;
    }

    // Move Assignment Operator
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {// Cleanup existing resources

            // Transfer ownership
            managedBuffer = other.managedBuffer;
            allocator = other.allocator;

            // Invalidate source
            other.managedBuffer = { VK_NULL_HANDLE, VK_NULL_HANDLE };
            other.allocator = VK_NULL_HANDLE;
        }
        return *this;
    }

    // Delete copy operations
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    virtual VkBuffer handle() const override {
        return managedBuffer.buffer;
    }

    virtual void destroy() override {
        if (managedBuffer.buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, managedBuffer.buffer, managedBuffer.allocation);
            managedBuffer.buffer = VK_NULL_HANDLE;
            managedBuffer.allocation = VK_NULL_HANDLE;
        }
    }

    ~Buffer() override {
        destroy();
    }
};