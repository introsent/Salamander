#pragma once
#include "buffer.h"
#include "buffer_manager.h"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>
#include "camera/camera.h"

class UniformBuffer final : public Buffer {
public:
    UniformBuffer() = default;
    UniformBuffer(BufferManager* bufferManager, VmaAllocator alloc, VkDeviceSize bufferSize);
    UniformBuffer(UniformBuffer&& other) noexcept;
    UniformBuffer& operator=(UniformBuffer&& other) noexcept;
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    ~UniformBuffer() override;

    // Updates buffer contents and flushes to GPU
    void update(VkDevice device, VkExtent2D extent, Camera* camera) const;

    void updateOmniLight() const;

protected:
    void*               mapped        = nullptr;
    VmaAllocator        allocator     = VK_NULL_HANDLE;
    VmaAllocation       allocation    = VK_NULL_HANDLE;
    VkDeviceSize        size          = 0;

private:
    void unmapBuffer();
};
