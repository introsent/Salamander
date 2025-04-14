#pragma once
#include "buffer.h"
#include "buffer_manager.h"
#include "../core/data_structures.h"
#include "vk_mem_alloc.h"
#include <vector>
class VertexBuffer final : public Buffer {
public:
    VertexBuffer() = default;
    VertexBuffer(BufferManager* bufferManager,
        const CommandManager* commandManager,
        VmaAllocator allocator,
        const std::vector<Vertex>& vertices);

    // Implement move operations
    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    // Disable copy
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    ~VertexBuffer() override = default;
};
