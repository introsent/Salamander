#pragma once
#include "ibuffer.h"
#include "buffer_manager.h"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>

class Buffer : public IBuffer {
public:
    ManagedBuffer managedBuffer = {};
    VmaAllocator allocator = VK_NULL_HANDLE;

    Buffer() = default;
    Buffer(VmaAllocator alloc) : allocator(alloc) {}
    Buffer(Buffer&& other) noexcept {
        managedBuffer = other.managedBuffer;
        allocator = other.allocator;
        other.managedBuffer = { VK_NULL_HANDLE, VK_NULL_HANDLE }; // Invalidate source
        other.allocator = VK_NULL_HANDLE;
    }
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            managedBuffer = other.managedBuffer;
            allocator = other.allocator;
            other.managedBuffer = { VK_NULL_HANDLE, VK_NULL_HANDLE };
            other.allocator = VK_NULL_HANDLE;
        }
        return *this;
    }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    ~Buffer() override = default;

    virtual VkBuffer handle() const override {
        return managedBuffer.buffer;
    }
};