#pragma once

#include "irender_pass.h"
#include "graphics/descriptors/descriptor_set_layout.h"
#include "graphics/descriptors/managers/main_descriptor_manager.h"
#include "renderer/passes/pass_dependencies.h"
#include "pipeline/compute_pipeline.h"
#include "scene/scene_data.h"
#include <memory>
#include <array>


namespace Salamander::Renderer::Passes {
    struct LuminanceHistogramPushConstants {
        float minLogLum;
        float inverseLogLumRange;
    };

    class LuminanceHistogramPass : public IRenderPass {
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

        void createAttachments();

        const Salamander::Renderer::RenderContext *m_ctx = nullptr;
        Salamander::Scene::MainSceneData *m_globalData = nullptr;
        Salamander::Renderer::PassDependencies *m_dependencies = nullptr;

        std::unique_ptr<ComputePipeline> m_pipeline;
        std::unique_ptr<DescriptorSetLayout> m_descriptorLayout;
        std::unique_ptr<MainDescriptorManager> m_descriptorManager;

        std::array<ManagedBuffer, Salamander::Renderer::MAX_FRAMES_IN_FLIGHT> m_histogramBuffers{};
    };
}
