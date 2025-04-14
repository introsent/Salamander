#pragma once
#include "buffer.h"
#include "buffer_manager.h"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>
#include "../core/data_structures.h"

class UniformBuffer final : public Buffer {
public:
    void* mapped = nullptr; 

    // Constructs the uniform buffer, mapping it persistently.
    UniformBuffer() = default;
    UniformBuffer(BufferManager* bufferManager, VmaAllocator alloc, VkDeviceSize bufferSize);


    UniformBuffer(UniformBuffer&& other) noexcept
        : Buffer(std::move(other)), // Invoke base move
        mapped(other.mapped)
    {
        other.mapped = nullptr; // Invalidate source
    }
    UniformBuffer& operator=(UniformBuffer&& other) noexcept {
        if (this != &other) {
            Buffer::operator=(std::move(other)); // Invoke base move
            mapped = other.mapped;
            other.mapped = nullptr;
        }
        return *this;
    }
    // Disable copying
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    // Updates uniform data; extent is used to compute the aspect ratio.
    void update(VkExtent2D extent) const;

    ~UniformBuffer() override;
};
