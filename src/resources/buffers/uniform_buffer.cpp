#include "uniform_buffer.h"

namespace Salamander::Resources::Buffers
{
    UniformBuffer::UniformBuffer(BufferManager *bufferManager, VmaAllocator alloc, VkDeviceSize bufferSize)
        : m_allocator(alloc), m_size(bufferSize) {
        managedBuffer = bufferManager->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );
        m_allocation = managedBuffer.allocation;
        vmaMapMemory(m_allocator, m_allocation, &m_mapped);
    }

    UniformBuffer::UniformBuffer(UniformBuffer &&other) noexcept
        : Buffer(std::move(other)), m_mapped(other.m_mapped), m_allocator(other.m_allocator),
          m_allocation(other.m_allocation), m_size(other.m_size) {
        other.m_mapped = nullptr;
        other.m_allocator = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
    }

    UniformBuffer &UniformBuffer::operator=(UniformBuffer &&other) noexcept {
        if (this != &other) {
            unmapBuffer();
            Buffer::operator=(std::move(other));
            m_mapped = other.m_mapped;
            m_allocator = other.m_allocator;
            m_allocation = other.m_allocation;
            m_size = other.m_size;
            other.m_mapped = nullptr;
            other.m_allocator = VK_NULL_HANDLE;
            other.m_allocation = VK_NULL_HANDLE;
        }
        return *this;
    }

    UniformBuffer::~UniformBuffer() {
        unmapBuffer();
    }


    void UniformBuffer::unmapBuffer() {
        if (m_mapped &&m_allocator
        !=
        VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE
        )
        {
            vmaUnmapMemory(m_allocator, m_allocation);
            m_mapped = nullptr;
        }
    }
}
