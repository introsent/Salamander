#pragma once
#include "shared/scene_data.h"
#include "target/render_target.h"

class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    virtual void initialize(const RenderTarget::SharedResources& shared,
                           MainSceneGlobalData& globalData,
                           PassDependencies& dependencies) = 0;
    virtual void cleanup() = 0;
    virtual void recreateSwapChain() = 0;
    virtual void execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) = 0;
};
