//
// Created by ivans on 30/05/2025.
//

#ifndef SALAMANDER_DEPTH_PREPASS_H
#define SALAMANDER_DEPTH_PREPASS_H


#include <memory>
#include "irender_pass.h"
#include "descriptors/descriptor_set_layout.h"
#include "descriptors/managers/main_descriptor_manager.h"
#include "frame/render_context.h"
#include "pipeline/pipeline.h"

namespace Salamander::Renderer::Passes {
    class DepthPrepass final : public IRenderPass {
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

        const Frame::RenderContext *m_ctx = nullptr;
        Scene::MainSceneData *m_globalData = nullptr;
        PassDependencies *m_dependencies = nullptr;

        std::unique_ptr<Graphics::Pipeline::Pipeline> m_pipeline;
        std::unique_ptr<Graphics::Descriptors::DescriptorSetLayout> m_descriptorLayout;
        std::unique_ptr<Graphics::Descriptors::MainDescriptorManager> m_descriptorManager;
    };
}


#endif //SALAMANDER_DEPTH_PREPASS_H