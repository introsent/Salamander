//
// Created by ivans on 20/03/2026.
//

#ifndef SALAMANDER_RENDER_CONTEXT_H
#define SALAMANDER_RENDER_CONTEXT_H


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
        );

        // getters with proper references
        [[nodiscard]] Core::Context& context() const { return m_context; }
        [[nodiscard]] Core::Window& window() const { return m_window; }
        [[nodiscard]] Core::SwapChain& swapChain() const { return m_swapChain; }
        [[nodiscard]] Resources::Buffers::CommandManager& commandManager() const { return m_commandManager; }
        [[nodiscard]] Resources::Buffers::BufferManager& bufferManager() const { return m_bufferManager; }
        [[nodiscard]] Resources::Textures::TextureManager& textureManager() const { return m_textureManager; }
        [[nodiscard]] VmaAllocator allocator() const { return m_allocator; }
        [[nodiscard]] VkImageView depthImageView() const { return m_depthImageView; }
        [[nodiscard]] VkFormat depthFormat() const { return m_depthFormat; }
        [[nodiscard]] Scene::Camera& camera() const { return m_camera; }
        [[nodiscard]] std::vector<Frame>& frames() const { return m_frames; }

    private:
        Core::Context& m_context;
        Core::Window& m_window;
        Core::SwapChain& m_swapChain;
        Resources::Buffers::CommandManager& m_commandManager;
        Resources::Buffers::BufferManager& m_bufferManager;
        Resources::Textures::TextureManager& m_textureManager;
        VmaAllocator m_allocator;
        VkImageView m_depthImageView;
        VkFormat m_depthFormat;
        Scene::Camera& m_camera;
        std::vector<Frame>& m_frames;
    };
}


#endif //SALAMANDER_RENDER_CONTEXT_H
