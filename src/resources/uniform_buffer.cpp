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

void UniformBuffer::update(VkExtent2D extent, Camera* camera) const
{
    // Build UBO
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view  = camera->GetViewMatrix();
    ubo.proj  = camera->GetProjectionMatrix(
        static_cast<float>(extent.width) / static_cast<float>(extent.height)
    );

    // Copy to mapped memory
    std::memcpy(mapped, &ubo, sizeof(ubo));

    // Flush via VMA so GPU sees writes
    vmaFlushAllocation(allocator, allocation, 0, sizeof(ubo));
}

void UniformBuffer::unmapBuffer()
{
    if (mapped && allocator != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE) {
        vmaUnmapMemory(allocator, allocation);
        mapped = nullptr;
    }
}