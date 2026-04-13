#include "main_scene_target.h"

namespace Salamander::Renderer::Targets {
    MainSceneTarget::MainSceneTarget(Passes::PassDependencies &dependencies) : m_controller(dependencies) {
    }

    void MainSceneTarget::initialize(const Frame::RenderContext &ctx) {
        m_controller.initialize(ctx);
    }

    void MainSceneTarget::render(float deltaTime, VkCommandBuffer cmd, uint32_t imageIndex, uint32_t frameIndex) {
        m_controller.render(deltaTime, cmd, imageIndex, frameIndex);
    }

    void MainSceneTarget::recreateSwapChain() {
        m_controller.recreateSwapChain();
    }

    void MainSceneTarget::cleanup() {
        m_controller.cleanup();
    }
}