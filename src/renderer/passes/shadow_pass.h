//
// Created by ivans on 03/06/2025.
//

#ifndef SALAMANDER_SHADOW_PASS_H
#define SALAMANDER_SHADOW_PASS_H


#include "irender_pass.h"
#include "graphics/pipeline/pipeline.h"
#include "graphics/descriptors/descriptor_set_layout.h"
#include "graphics/descriptors/managers/main_descriptor_manager.h"
#include "renderer/frame/render_context.h"
#include "renderer/frame/frame_data.h"
#include "resources/buffers/uniform_buffer.h"
#include "lighting/lights.h"
#include <memory>

namespace Salamander::Renderer::Passes {

    struct ShadowPushConstants {
        uint64_t vertexBufferAddress;
        glm::vec3 modelScale;
        uint32_t baseColorTextureIndex;
    };

    class ShadowPass final : public IRenderPass {
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

        static constexpr glm::vec3 globalScale{1.0f};
        static constexpr int MAX_FRAMES_IN_FLIGHT = Frame::MAX_FRAMES_IN_FLIGHT;
    };
}


#endif //SALAMANDER_SHADOW_PASS_H