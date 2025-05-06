#pragma once
#include "context.h"
#include "window.h"
#include "target/render_target.h"
#include <memory>
#include <vector>
#include "image_views.h"
#include "depth_format.h"

class Renderer {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    Renderer(Context* context, Window* window, VmaAllocator allocator);
    ~Renderer();

    void drawFrame();
    void recreateSwapChain();
    void markFramebufferResized();

private:
    struct Frame {
        std::unique_ptr<CommandBuffer> commandBuffer;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
    };

    void createSyncObjects();
    void createCommandBuffers();
    void cleanup();
    void initializeSharedResources();

    Context* m_context;
    Window* m_window;
    VmaAllocator m_allocator;
    bool m_framebufferResized = false;

    // Shared Resources
    RenderTarget::SharedResources m_sharedResources;

    // Managers
    std::unique_ptr<SwapChain> m_swapChain;
    std::unique_ptr<CommandManager> m_commandManager;
    std::unique_ptr<BufferManager> m_bufferManager;
    std::unique_ptr<TextureManager> m_textureManager;

    // Images
    std::unique_ptr<DepthFormat> m_depthFormat;
    ManagedTexture m_depthImage;

    // Targets
    std::vector<std::unique_ptr<RenderTarget>> m_renderTargets;

    // Frame resources
    std::vector<Frame> m_frames;
    uint32_t m_currentFrame = 0;
};
