#pragma once
#include "context.h"
#include "swap_chain.h"
#include "command_manager.h"
#include "texture_manager.h"
#include "render_pass_executor.h"

class RenderPassExecutor;

class RenderTarget {
public:
    virtual ~RenderTarget() = default;
    
    virtual void initialize(const SharedResources& shared) = 0;
    virtual void render(VkCommandBuffer commandBuffer, uint32_t imageIndex) = 0;
    virtual void recreateSwapChain() = 0;
    virtual void cleanup() = 0;
    virtual void updateUniformBuffers() const = 0;

protected:
    const SharedResources* m_shared = nullptr;
    std::unique_ptr<RenderPassExecutor> m_executor;
};