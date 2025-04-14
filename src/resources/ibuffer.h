#pragma once
#include <vulkan/vulkan.h>

// IBuffer serves as an interface for all buffer types.
class IBuffer {
public:
    // Return the underlying Vulkan buffer handle.
    virtual VkBuffer getBuffer() const = 0;

    // Clean up the resources held by the buffer.
    virtual void destroy() = 0;

    virtual ~IBuffer() {}
};
