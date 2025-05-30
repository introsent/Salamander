#pragma once
#include "context.h"
#include "swap_chain.h"
#include "window.h"
#include "command_manager.h"
#include "buffer_manager.h"
#include "data_structures.h"
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
        uint32_t* currentFrame;
        VmaAllocator allocator;
        VkImageView depthImageView;
        VkImage depthImage;
        VkFormat depthFormat;
        Camera* camera;
        std::vector<Frame>* frames;
    };


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