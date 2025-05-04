#pragma once
#include <vulkan/vulkan.h>
class RenderPassExecutor {
public:
    virtual void begin(VkCommandBuffer cmd, uint32_t imageIndex) = 0;
    virtual void execute(VkCommandBuffer cmd) = 0;
    virtual void end(VkCommandBuffer cmd) = 0;
    virtual ~RenderPassExecutor() = default;
};
