#pragma once
#include "context.h"
#include "swap_chain.h"
#include "window.h"
#include "render_pass.h"
#include "command_manager.h"
#include "buffer_manager.h"
#include "texture_manager.h"
#include "framebuffer_manager.h"
#include "render_pass_executor.h"

class RenderPassExecutor;

class RenderTarget {
public:
    struct SharedResources {
        Context* context;
        Window* window;
        SwapChain* swapChain;
        CommandManager* commandManager;
        BufferManager* bufferManager;
        TextureManager* textureManager;
        FramebufferManager* framebufferManager;
        uint32_t currentFrame;
        VmaAllocator allocator;
    };

    virtual ~RenderTarget() = default;
    
    virtual void initialize(const SharedResources& shared) = 0;
    virtual void render(VkCommandBuffer commandBuffer, uint32_t imageIndex) = 0;
    virtual void recreateSwapChain() = 0;
    virtual void cleanup() = 0;

protected:
    SharedResources m_shared = {};
    std::unique_ptr<RenderPass> m_renderPass;
    FramebufferManager::FramebufferSet m_framebuffers;
    std::unique_ptr<RenderPassExecutor> m_executor;
};
