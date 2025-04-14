#include "index_buffer.h"

IndexBuffer::IndexBuffer(BufferManager* bufferManager,
    const CommandManager* commandManager,
    VmaAllocator alloc,
    const std::vector<uint32_t>& indices)
    : Buffer(alloc)  // Pass allocator to the base class
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    // Create staging buffer (CPU visible).
    ManagedBuffer staging = bufferManager->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    void* data;
    vmaMapMemory(allocator, staging.allocation, &data);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vmaUnmapMemory(allocator, staging.allocation);

    // Create a GPU-only index buffer.
    managedBuffer = bufferManager->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // Copy data from the staging buffer.
    VkCommandBuffer commandBuffer = commandManager->beginSingleTimeCommands();
    bufferManager->copyBuffer(staging.buffer, managedBuffer.buffer, bufferSize);
    commandManager->endSingleTimeCommands(commandBuffer);
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : Buffer(std::move(other)) {
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
    if (this != &other) {
        Buffer::operator=(std::move(other));
    }
    return *this;
}