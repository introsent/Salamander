#pragma once

#include "irender_pass.h"
#include "graphics/pipeline/compute_pipeline.h"
#include "graphics/descriptors/descriptor_set_layout.h"
#include "graphics/descriptors/managers/main_descriptor_manager.h"
#include "renderer/frame/render_context.h"
#include "renderer/frame/frame_data.h"
#include <memory>
#include <array>

namespace Salamander::Renderer::Passes {
    struct LuminanceHistogramPushConstants {
        float minLogLum;
        float inverseLogLumRange;
    };

    class LuminanceHistogramPass final : public IRenderPass {
    public:
        void initialize(const Frame::RenderContext &ctx,
                        Scene::MainSceneData &globalData,
                        PassDependencies &dependencies) override;
        void cleanup() override;
        void recreateSwapChain() override;
        void execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) override;

    private:
        void createPipeline();
        void createDescriptors();
        void createAttachments();
        void updateDescriptors() const;

        const Frame::RenderContext *m_ctx = nullptr;
        Scene::MainSceneData *m_globalData = nullptr;
        PassDependencies *m_dependencies = nullptr;

        std::unique_ptr<Graphics::Pipeline::ComputePipeline> m_pipeline;
        std::unique_ptr<Graphics::Descriptors::DescriptorSetLayout> m_descriptorLayout;
        std::unique_ptr<Graphics::Descriptors::MainDescriptorManager> m_descriptorManager;

        std::array<Resources::Buffers::ManagedBuffer, Frame::MAX_FRAMES_IN_FLIGHT> m_histogramBuffers{};

        static constexpr int MAX_FRAMES_IN_FLIGHT = Frame::MAX_FRAMES_IN_FLIGHT;
    };
}