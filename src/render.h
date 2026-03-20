#pragma once
#include "context.h"
#include "window.h"
#include <memory>
#include <vector>
#include <vk_mem_alloc.h>

#include "depth_format.h"


namespace Salamander {
    namespace Renderer::Frame {
        struct Frame;
    }

    namespace Renderer::Targets {
        class RenderTarget;
    }

    namespace Resources::Textures {
        class TextureManager;
    }

    namespace Resources::Buffers {
        class BufferManager;
        class CommandManager;
    }

    namespace Core {
        class SwapChain;
    }

    class Render {
    public:
        Render(Core::Context* context, Core::Window* window, VmaAllocator allocator,
                 Scene::Camera* camera);
        ~Render();

        void drawFrame(float deltaTime);
        void recreateSwapChain();
        void markFramebufferResized();

    private:
        void createSyncObjects();
        void createCommandBuffers();
        void cleanup();
        void initializeSharedResources(Scene::Camera* camera);

        Core::Context *m_context;
        Core::Window *m_window;
        VmaAllocator m_allocator;
        bool m_framebufferResized = false;

        SharedResources m_sharedResources{};

        uint32_t m_swapchainVersion = 0;

        // Managers
        std::unique_ptr<Core::SwapChain> m_swapChain;
        std::unique_ptr<Resources::Buffers::CommandManager> m_commandManager;
        std::unique_ptr<Resources::Buffers::BufferManager> m_bufferManager;
        std::unique_ptr<Resources::Textures::TextureManager> m_textureManager;

        // Images
        std::unique_ptr<Graphics::DepthFormat> m_depthFormat;

        // Targets
        std::vector<std::unique_ptr<Renderer::Targets::RenderTarget> > m_renderTargets{};

        // Frame resources
        std::vector<Renderer::Frame::Frame> m_frames{};
        uint32_t m_currentFrame = 0;
    };
}
