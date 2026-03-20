#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <vector>
#include "context.h"
#include "window.h"
#include "swap_chain.h"

namespace Salamander {
    namespace Resources::Buffers { class BufferManager; }
    namespace Resources::Textures { class TextureManager; }
    namespace Resources::Buffers { class CommandManager; }
}

namespace Salamander::Renderer::Frame {
    // forward declarations
    class Camera;
    struct Frame;

    /// RenderContext holds all persistent rendering resources that live for the lifetime of the renderer
    class RenderContext {
    public:
        RenderContext(
            Context& context,
            Window& window,
            SwapChain& swapChain,
            Resources::Buffers::CommandManager& commandManager,
            Resources::Buffers::BufferManager& bufferManager,
            Resources::Textures::TextureManager& textureManager,
            VmaAllocator allocator,
            VkImageView depthImageView,
            VkFormat depthFormat,
            Camera& camera,
            std::vector<Frame>& frames
        );

        // getters with proper references
        [[nodiscard]] Context& context() const { return m_context; }
        [[nodiscard]] Window& window() const { return m_window; }
        [[nodiscard]] SwapChain& swapChain() const { return m_swapChain; }
        [[nodiscard]] Resources::Buffers::CommandManager& commandManager() const { return m_commandManager; }
        [[nodiscard]] Resources::Buffers::BufferManager& bufferManager() const { return m_bufferManager; }
        [[nodiscard]] Resources::Textures::TextureManager& textureManager() const { return m_textureManager; }
        [[nodiscard]] VmaAllocator allocator() const { return m_allocator; }
        [[nodiscard]] VkImageView depthImageView() const { return m_depthImageView; }
        [[nodiscard]] VkFormat depthFormat() const { return m_depthFormat; }
        [[nodiscard]] Camera& camera() const { return m_camera; }
        [[nodiscard]] std::vector<Frame>& frames() const { return m_frames; }

    private:
        Context& m_context;
        Window& m_window;
        SwapChain& m_swapChain;
        Resources::Buffers::CommandManager& m_commandManager;
        Resources::Buffers::BufferManager& m_bufferManager;
        Resources::Textures::TextureManager& m_textureManager;
        VmaAllocator m_allocator;
        VkImageView m_depthImageView;
        VkFormat m_depthFormat;
        Camera& m_camera;
        std::vector<Frame>& m_frames;
    };

}
