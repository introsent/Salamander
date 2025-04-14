#pragma once
#include "buffer.h"
#include "buffer_manager.h"
#include "../core/data_structures.h"
#include "vk_mem_alloc.h"
#include <vector>
class VertexBuffer : public Buffer {
public:
    VertexBuffer(BufferManager* bufferManager,
        const CommandManager* commandManager,
        VmaAllocator allocator,
        const std::vector<Vertex>& vertices);
};
