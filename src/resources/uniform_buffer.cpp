#include "uniform_buffer.h"
#include "data_structures.h"
#include <cstring>

UniformBuffer::UniformBuffer(BufferManager* bufferManager, VmaAllocator alloc, VkDeviceSize bufferSize)
    : allocator(alloc), size(bufferSize)
{
    managedBuffer = bufferManager->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    allocation = managedBuffer.allocation;
    vmaMapMemory(allocator, allocation, &mapped);
}

UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    : Buffer(std::move(other)), mapped(other.mapped), allocator(other.allocator), allocation(other.allocation), size(other.size)
{
    other.mapped = nullptr;
    other.allocator = VK_NULL_HANDLE;
    other.allocation = VK_NULL_HANDLE;
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept
{
    if (this != &other) {
        unmapBuffer();
        Buffer::operator=(std::move(other));
        mapped      = other.mapped;
        allocator   = other.allocator;
        allocation  = other.allocation;
        size        = other.size;
        other.mapped     = nullptr;
        other.allocator  = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
    }
    return *this;
}

UniformBuffer::~UniformBuffer() {
    unmapBuffer();
}


void UniformBuffer::unmapBuffer()
{
    if (mapped && allocator != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE) {
        vmaUnmapMemory(allocator, allocation);
        mapped = nullptr;
    }
}
