//
// Created by ivans on 01/04/2026.
//

#ifndef SALAMANDER_PASS_NODE_H
#define SALAMANDER_PASS_NODE_H
#include <functional>
#include <string>
#include <vulkan/vulkan.h>

#include "barrier_descriptor.h"
#include "render_graph/resource_access.h"

namespace Salamander::Renderer::RenderGraph::Internal {
    // Pass node is graph's knowledge about the pass
    struct ResourceReference {
        uint32_t resourceIndex;
        ResourceAccess access;
        VkPipelineStageFlags2 stageHint = 0;
    };

    struct PassNode {
        std::string name{};
        VkPipelineStageFlags2 stageFlags{};
        std::vector<ResourceReference> resourceReferences{};
        std::function<void(VkCommandBuffer, uint32_t /*frameIndex*/, uint32_t /*imageIndex*/)> executeCallback{};
        std::vector<BarrierDescriptor> barriers{}; // barriers to be executed
        bool culled = true;
    };


}


#endif //SALAMANDER_PASS_NODE_H