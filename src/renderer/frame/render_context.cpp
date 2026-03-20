#include "render_context.h"

namespace Salamander::Renderer::Frame {
    RenderContext::RenderContext(
        Context& context,
        Window& window,
        SwapChain& swapChain,
        CommandManager& commandManager,
        BufferManager& bufferManager,
        TextureManager& textureManager,
        VmaAllocator allocator,
        VkImageView depthImageView,
        VkFormat depthFormat,
        Camera& camera,
        std::vector<Frame>& frames
    )
        : m_context(context)
        , m_window(window)
        , m_swapChain(swapChain)
        , m_commandManager(commandManager)
        , m_bufferManager(bufferManager)
        , m_textureManager(textureManager)
        , m_allocator(allocator)
        , m_depthImageView(depthImageView)
        , m_depthFormat(depthFormat)
        , m_camera(camera)
        , m_frames(frames)
    {
    }
}
