#pragma once
#include "buffer.h"
#include "buffer_manager.h"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>

#include "camera/camera.h"

class UniformBuffer final : public Buffer {
public:
   

    // Constructs the uniform buffer, mapping it persistently.
    UniformBuffer() = default;
    UniformBuffer(BufferManager* bufferManager, VmaAllocator alloc, VkDeviceSize bufferSize);
    UniformBuffer(UniformBuffer&& other) noexcept;
    UniformBuffer& operator=(UniformBuffer&& other) noexcept;
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    ~UniformBuffer() override;

    void update(VkExtent2D extent, Camera*) const;

protected:
    void* mapped = nullptr;
private:
    void unmapBuffer();

    
};
