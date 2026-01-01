#pragma once
#include "context.h"
#include "window.h"
#include <memory>
#include <vector>
#include <vk_mem_alloc.h>

#include "depth_format.h"
#include "shared/shared_resources.h"

// Forward declarations instead of includes
class RenderTarget;
class SwapChain;
class CommandManager;
class BufferManager;
class TextureManager;
class Camera;
struct Frame;

class Renderer {
public:

    Renderer(Context* context, Window* window, VmaAllocator allocator, Camera* camera);
    ~Renderer();

    void drawFrame();
    void recreateSwapChain();
    void markFramebufferResized();

private:

    void createSyncObjects();
    void createCommandBuffers();
    void cleanup();
    void initializeSharedResources(Camera* camera);

    Context* m_context;
    Window* m_window;
    VmaAllocator m_allocator;
    bool m_framebufferResized = false;

    SharedResources m_sharedResources;

    uint32_t m_swapchainVersion = 0;

    // Managers
    std::unique_ptr<SwapChain> m_swapChain;
    std::unique_ptr<CommandManager> m_commandManager;
    std::unique_ptr<BufferManager> m_bufferManager;
    std::unique_ptr<TextureManager> m_textureManager;

    // Images
    std::unique_ptr<DepthFormat> m_depthFormat;

    // Targets
    std::vector<std::unique_ptr<RenderTarget>> m_renderTargets;

    // Frame resources
    std::vector<Frame> m_frames;
    uint32_t m_currentFrame = 0;
};