#pragma once
#include <memory>
#include "irender_pass.h"
#include "descriptors/descriptor_set_layout.h"
#include "descriptors/managers/main_descriptor_manager.h"
#include "frame/render_context.h"
#include "pipeline/pipeline.h"

namespace Salamander::Renderer::Passes {
    class DepthPrepass : public IRenderPass {
    public:
        void initialize(const Salamander::Renderer::RenderContext &ctx,
                        Salamander::Scene::MainSceneData &globalData,
                        Salamander::Renderer::PassDependencies &dependencies) override;
        void cleanup() override;
        void recreateSwapChain() override;
        void execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) override;

    private:
        void createPipeline();
        void createDescriptors();

        const Salamander::Renderer::RenderContext *m_ctx = nullptr;
        Salamander::Scene::MainSceneData *m_globalData = nullptr;
        Salamander::Renderer::PassDependencies *m_dependencies = nullptr;

        std::unique_ptr<Pipeline> m_pipeline;
        std::unique_ptr<DescriptorSetLayout> m_descriptorLayout;
        std::unique_ptr<MainDescriptorManager> m_descriptorManager;
    };
}
