#include "ssbo_buffer.h"

SSBOBuffer::SSBOBuffer(
    BufferManager* bufferManager,
    const CommandManager* commandManager,
    VmaAllocator alloc,
    const void* data,
    VkDeviceSize dataSize
) : Buffer(alloc) {
    // Create staging buffer (CPU visible)
    ManagedBuffer staging = bufferManager->createBuffer(
        dataSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    // Map and copy data
    void* mappedData;
    vmaMapMemory(alloc, staging.allocation, &mappedData);
    memcpy(mappedData, data, dataSize);
    vmaUnmapMemory(alloc, staging.allocation);

    // Create SSBO with device address support
    m_ssboBuffer = bufferManager->createBuffer(
        dataSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, // critical for vertex SSBO
        VMA_MEMORY_USAGE_AUTO
    ).buffer;

    // Copy staging -> SSBO
    VkCommandBuffer cmd = commandManager->beginSingleTimeCommands();
    bufferManager->copyBuffer(staging.buffer, m_ssboBuffer, dataSize);
    commandManager->endSingleTimeCommands(cmd);
}

uint64_t SSBOBuffer::getDeviceAddress(VkDevice device) const {
    VkBufferDeviceAddressInfo addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = m_ssboBuffer;
    return vkGetBufferDeviceAddress(device, &addressInfo);  // Requires enabled feature
}