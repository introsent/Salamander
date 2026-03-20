#pragma once
#include "buffer.h"
#include "buffer_manager.h"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>
#include "camera/camera.h"

namespace Salamander::Resources::Buffers
{
    class UniformBuffer final : public Buffer {
        public:
            UniformBuffer() = default;
            UniformBuffer(BufferManager * bufferManager, VmaAllocator alloc, VkDeviceSize bufferSize);
            UniformBuffer(UniformBuffer && other) noexcept;
            UniformBuffer & operator=(UniformBuffer && other) noexcept;
            UniformBuffer(const UniformBuffer &) = delete;
            UniformBuffer & operator=(const UniformBuffer &) = delete;
            ~UniformBuffer()override;

            template <typename T>
            void update(const T &data) const
            {
                std::memcpy(m_mapped, &data, sizeof(T));
            }
        protected:
            void *m_mapped = nullptr;
            VmaAllocator m_allocator = VK_NULL_HANDLE;
            VmaAllocation m_allocation = VK_NULL_HANDLE;
            VkDeviceSize m_size = 0;

        private:
            void unmapBuffer();
    };
}
