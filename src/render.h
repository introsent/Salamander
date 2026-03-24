#pragma once
#include "context.h"
#include "window.h"
#include <memory>
#include <vector>
#include <vk_mem_alloc.h>

#include "depth_format.h"
#include "renderer/frame/render_context.h"

#include "passes/pass_dependencies.h"


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
        void initializeResources(Scene::Camera* camera);

        Core::Context *m_context;
        Core::Window *m_window;
        VmaAllocator m_allocator;
        Scene::Camera *m_camera;
        bool m_framebufferResized = false;

        uint32_t m_swapchainVersion = 0;

        // Managers
        std::unique_ptr<Core::SwapChain> m_swapChain;
        std::unique_ptr<Resources::Buffers::CommandManager> m_commandManager;
        std::unique_ptr<Resources::Buffers::BufferManager> m_bufferManager;
        std::unique_ptr<Resources::Textures::TextureManager> m_textureManager;

        // Images
        std::unique_ptr<Graphics::DepthFormat> m_depthFormat;

        // Render context
        std::unique_ptr<Renderer::Frame::RenderContext> m_renderContext;

        // Targets
        std::vector<std::unique_ptr<Renderer::Targets::RenderTarget> > m_renderTargets{};

        // Frame resources
        std::vector<Renderer::Frame::Frame> m_frames{};
        uint32_t m_currentFrame = 0;

        // Pass dependencies
        Renderer::Passes::PassDependencies m_dependencies;
    };
}
