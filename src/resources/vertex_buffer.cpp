#include "vertex_buffer.h"

#include "command_manager.h"

VertexBuffer::VertexBuffer(BufferManager* bufferManager,
                           const CommandManager* commandManager,
                           VmaAllocator alloc,
                           const std::vector<Vertex>& vertices)
    : Buffer(alloc)  // Pass allocator to the base class
{
    managedBuffer.buffer = VK_NULL_HANDLE;
    managedBuffer.allocation = nullptr;

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    // Create staging buffer (CPU visible).
    ManagedBuffer staging = bufferManager->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    void* data;
    vmaMapMemory(allocator, staging.allocation, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vmaUnmapMemory(allocator, staging.allocation);

    // Create a GPU-only vertex buffer.
    managedBuffer = bufferManager->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );
    // Copy data from staging buffer into the GPU buffer.
    VkCommandBuffer commandBuffer = commandManager->beginSingleTimeCommands();
    bufferManager->copyBuffer(staging.buffer, managedBuffer.buffer, bufferSize);
    commandManager->endSingleTimeCommands(commandBuffer);
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : Buffer(std::move(other)) {
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        Buffer::operator=(std::move(other)); 
    }
    return *this;
}
