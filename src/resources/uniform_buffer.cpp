#include "uniform_buffer.h"

UniformBuffer::UniformBuffer(BufferManager* bufferManager, VmaAllocator alloc, VkDeviceSize bufferSize)
    : Buffer(alloc) // Pass allocator to the base class
{
    // Create the uniform buffer (CPU accessible)
    managedBuffer = bufferManager->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    // Map the buffer persistently.
    vmaMapMemory(allocator, managedBuffer.allocation, &mapped);
}

void UniformBuffer::update(VkExtent2D extent) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f),
        time * glm::radians(90.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),
        static_cast<float>(extent.width) / static_cast<float>(extent.height),
        0.1f, 10.0f);
    ubo.proj[1][1] *= -1; // Flip Y for Vulkan

    memcpy(mapped, &ubo, sizeof(ubo));
}

UniformBuffer::~UniformBuffer() {
    if (mapped) {
        vmaUnmapMemory(allocator, managedBuffer.allocation);
        mapped = nullptr;
    }
    // Call the base class's destroy function.
    destroy();
}
