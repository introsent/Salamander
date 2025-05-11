#include "uniform_buffer.h"
#include <chrono>
#include "data_structures.h"

UniformBuffer::UniformBuffer(BufferManager* bufferManager, VmaAllocator alloc, VkDeviceSize bufferSize)
    : Buffer(alloc) // Pass allocator to the base class
{
    managedBuffer = bufferManager->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    vmaMapMemory(allocator, managedBuffer.allocation, &mapped);
}

UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    : Buffer(std::move(other)),
    mapped(other.mapped)
{
    other.mapped = nullptr; // Invalidate source
}
UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept
{
    if (this != &other) {
        unmapBuffer();
        Buffer::operator=(std::move(other)); 
        mapped = other.mapped;
        other.mapped = nullptr;
    }
    return *this;
}

void UniformBuffer::update(VkExtent2D extent, Camera* camera) const
{
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera->GetViewMatrix();
    ubo.proj = camera->GetProjectionMatrix(static_cast<float>(extent.width) /
        static_cast<float>(extent.height));

    memcpy(mapped, &ubo, sizeof(ubo));
}

void UniformBuffer::unmapBuffer()
{
    if (mapped) {
        vmaUnmapMemory(allocator, managedBuffer.allocation);
        mapped = nullptr;
    }
}
UniformBuffer::~UniformBuffer() {
    unmapBuffer();
}
