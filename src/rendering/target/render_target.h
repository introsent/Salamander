#pragma once
#include "../core/context.h"
#include "../core/swap_chain.h"
#include "../core/window.h"
#include "../rendering/render_pass.h"
#include "../resources/command_manager.h"
#include "../resources/buffer_manager.h"
#include "../resources/texture_manager.h"
#include "../core/framebuffer_manager.h"

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
