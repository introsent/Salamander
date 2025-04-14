#pragma once
#include "buffer.h"
#include "buffer_manager.h"
#include "command_manager.h"
#include "vk_mem_alloc.h"
#include <vector>

class IndexBuffer final : public Buffer {
public:
    IndexBuffer() = default;
    // Constructs the index buffer from a vector of indices.
    IndexBuffer(BufferManager* bufferManager,
        const CommandManager* commandManager,
        VmaAllocator allocator,
        const std::vector<uint32_t>& indices);
    ~IndexBuffer() override = default;
    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;
    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
};
