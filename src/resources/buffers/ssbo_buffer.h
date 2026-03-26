//
// Created by ivans on 07/05/2025.
//

#ifndef SALAMANDER_SSBO_BUFFER_H
#define SALAMANDER_SSBO_BUFFER_H


#include "buffer.h"
#include "buffer_manager.h"
#include "command_manager.h"

namespace Salamander::Resources::Buffers
{
    class SSBOBuffer : public Buffer {
    public:
        SSBOBuffer() = default;
        SSBOBuffer(
            BufferManager * bufferManager,

        const CommandManager *commandManager, VmaAllocator alloc, const void *data, VkDeviceSize dataSize);

        // Get the GPU device address (critical for vertex pulling)
        uint64_t getDeviceAddress(VkDevice device) const;

    private:
        VkBuffer m_ssboBuffer{};
    };
}


#endif //SALAMANDER_SSBO_BUFFER_H
