#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include "frame/render_context.h"

namespace Salamander::Scene {
    struct MainSceneData;
}

/// Interface for render passes
/// All passes implement this interface to provide a consistent API
namespace Salamander::Renderer::Passes {
    struct PassDependencies;

    class IRenderPass {
    public:
        virtual
        ~IRenderPass() = default;

        virtual void initialize(const Frame::RenderContext &ctx,
                                Scene::MainSceneData &globalData,
                                PassDependencies &dependencies) = 0;
        virtual void cleanup() = 0;
        virtual void recreateSwapChain() = 0;
        virtual void execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) = 0;
    };
}
