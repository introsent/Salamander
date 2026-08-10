//
// Created by ivans on 30/05/2025.
//

#ifndef SALAMANDER_MAIN_SCENE_TARGET_H
#define SALAMANDER_MAIN_SCENE_TARGET_H


#include "renderer/targets/target/render_target.h"
#include "main_scene_controller.h"

namespace Salamander::Renderer::Targets {
    class MainSceneTarget final : public RenderTarget {
    public:
        explicit MainSceneTarget(Passes::PassDependencies& dependencies);
        void initialize(const Frame::RenderContext &ctx) override;
        void render(float deltaTime, VkCommandBuffer cmd, uint32_t imageIndex, uint32_t frameIndex) override;
        void recreateSwapChain() override;
        void cleanup() override;

        [[nodiscard]] RenderGraph::Graph& getRenderGraph() {
            return m_controller.getRenderGraph();
        }

    private:
        MainSceneController m_controller;
    };
}


#endif //SALAMANDER_MAIN_SCENE_TARGET_H