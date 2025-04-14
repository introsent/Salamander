#pragma once
#include "buffer.h"
#include "buffer_manager.h"
#include "command_manager.h"
#include "vk_mem_alloc.h"
#include <vector>

class IndexBuffer : public Buffer {
public:
    // Constructs the index buffer from a vector of indices.
    IndexBuffer(BufferManager* bufferManager,
        const CommandManager* commandManager,
        VmaAllocator allocator,
        const std::vector<uint32_t>& indices);
};
