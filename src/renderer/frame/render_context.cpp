#include "render_context.h"

#include "textures/texture_manager.h"

namespace Salamander::Renderer::Frame {
    RenderContext::RenderContext(
        Core::Context& context,
        Core::Window& window,
        Core::SwapChain& swapChain,
        Resources::Buffers::CommandManager& commandManager,
        Resources::Buffers::BufferManager& bufferManager,
        Resources::Textures::TextureManager& textureManager,
        VmaAllocator allocator,
        VkImageView depthImageView,
        VkFormat depthFormat,
        Scene::Camera& camera,
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
