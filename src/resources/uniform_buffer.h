#pragma once
#include "buffer.h"
#include "buffer_manager.h"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>
#include <chrono>
#include <cstring>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

// Define UniformBufferObject with model, view, and projection matrices.
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class UniformBuffer : public Buffer {
public:
    void* mapped = nullptr; 

    // Constructs the uniform buffer, mapping it persistently.
    UniformBuffer(BufferManager* bufferManager, VmaAllocator alloc, VkDeviceSize bufferSize);

    // Updates uniform data; extent is used to compute the aspect ratio.
    void update(VkExtent2D extent);

    ~UniformBuffer() override;
};
