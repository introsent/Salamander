#pragma once
#include <vulkan/vulkan.h>

namespace Salamander::Resources::Buffers {
    class CommandBuffer {
        public
        :
        CommandBuffer(VkCommandBuffer handle, VkCommandPool pool);

        void begin(VkCommandBufferUsageFlags flags = 0) const;
        void end() const;
        void reset() const;

        [[nodiscard]] VkCommandBuffer handle() const
        {
            return m_handle;
        }

        private
        :
        VkCommandBuffer m_handle;
        VkCommandPool m_pool;
    };
}
