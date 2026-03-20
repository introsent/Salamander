#pragma once
#include "buffer.h"
#include "buffer_manager.h"
#include "vk_mem_alloc.h"
#include <vector>
#include "components/vertex.h"

namespace Salamander::Resources::Buffers
{
    class VertexBuffer final : public Buffer {
        public:
            VertexBuffer() = default;
            VertexBuffer(BufferManager * bufferManager, const CommandManager *commandManager, VmaAllocator allocator,
                        const std::vector<Salamander::Scene::Vertex> &vertices);
            VertexBuffer(VertexBuffer && other) noexcept;
            VertexBuffer & operator=(VertexBuffer && other) noexcept;
            VertexBuffer(const VertexBuffer &) = delete;
            VertexBuffer & operator=(const VertexBuffer &) = delete;
        };
}
