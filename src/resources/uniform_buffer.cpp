#include "uniform_buffer.h"
#include "data_structures.h"
#include <cstring>

UniformBuffer::UniformBuffer(BufferManager* bufferManager, VmaAllocator alloc, VkDeviceSize bufferSize)
    : allocator(alloc), size(bufferSize)
{
    managedBuffer = bufferManager->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    allocation = managedBuffer.allocation;
    vmaMapMemory(allocator, allocation, &mapped);
}

UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    : Buffer(std::move(other)), mapped(other.mapped), allocator(other.allocator), allocation(other.allocation), size(other.size)
{
    other.mapped = nullptr;
    other.allocator = VK_NULL_HANDLE;
    other.allocation = VK_NULL_HANDLE;
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept
{
    if (this != &other) {
        unmapBuffer();
        Buffer::operator=(std::move(other));
        mapped      = other.mapped;
        allocator   = other.allocator;
        allocation  = other.allocation;
        size        = other.size;
        other.mapped     = nullptr;
        other.allocator  = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
    }
    return *this;
}

UniformBuffer::~UniformBuffer() {
    unmapBuffer();
}

void UniformBuffer::update(VkDevice device, VkExtent2D extent, Camera* camera) const
{
    // Build UBO
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view  = camera->GetViewMatrix();
    ubo.proj  = camera->GetProjectionMatrix(
        static_cast<float>(extent.width) / static_cast<float>(extent.height)
    );
    ubo.cameraPosition = camera->Position;
    // Copy to mapped memory
    std::memcpy(mapped, &ubo, sizeof(ubo));
}

void UniformBuffer::updateOmniLight() const {
    PointLightData omniLight{};
    omniLight.pointLightPosition = glm::vec3( glm::vec4(8.0f, 1.0f, 0.0f, 1.f));
    omniLight.pointLightIntensity = 1000.f;
    omniLight.pointLightColor = glm::vec3(0.f, 0.f, 1.f);
    omniLight.pointLightRadius = 4.f;

    std::memcpy(mapped, &omniLight, sizeof(omniLight));
}

void UniformBuffer::unmapBuffer()
{
    if (mapped && allocator != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE) {
        vmaUnmapMemory(allocator, allocation);
        mapped = nullptr;
    }
}
