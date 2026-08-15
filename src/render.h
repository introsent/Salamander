//
// Created by ivans on 30/05/2025.
//

#ifndef SALAMANDER_RENDER_H
#define SALAMANDER_RENDER_H


#include "context.h"
#include "window.h"
#include <memory>
#include <vector>
#include <vk_mem_alloc.h>

#include "depth_format.h"
#include "debug/debug_panel_registry.h"
#include "renderer/frame/render_context.h"

#include "passes/pass_dependencies.h"


namespace Salamander {
    namespace Renderer::Debug {
        class RenderGraphDebugPanel;
    }

    namespace Renderer::Frame {
        struct Frame;
    }

    namespace Renderer::Targets {
        class ImGuiTarget;
        class MainSceneTarget;
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
        void cleanup() const;
        void initializeResources(Scene::Camera* camera);

        Core::Context *m_context;
        Core::Window *m_window;
        VmaAllocator m_allocator;
        Scene::Camera *m_camera;
        bool m_framebufferResized = false;

        uint32_t m_swapchainVersion = 0;

        // managers
        std::unique_ptr<Core::SwapChain> m_swapChain;
        std::unique_ptr<Resources::Buffers::CommandManager> m_commandManager;
        std::unique_ptr<Resources::Buffers::BufferManager> m_bufferManager;
        std::unique_ptr<Resources::Textures::TextureManager> m_textureManager;

        // images
        std::unique_ptr<Graphics::DepthFormat> m_depthFormat;

        // render context
        std::unique_ptr<Renderer::Frame::RenderContext> m_renderContext;

        // targets
        std::vector<std::unique_ptr<Renderer::Targets::RenderTarget> > m_renderTargets{};

        // frame resources
        std::vector<Renderer::Frame::Frame> m_frames{};
        uint32_t m_currentFrame = 0;

        // pass dependencies
        Renderer::Passes::PassDependencies m_dependencies;

        Renderer::Debug::DebugPanelRegistry m_debugUI;
        Renderer::Targets::MainSceneTarget* m_mainSceneTarget = nullptr;
        Renderer::Targets::ImGuiTarget* m_imguiTarget = nullptr;
    };
}


#endif //SALAMANDER_RENDER_H
