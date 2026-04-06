//
// Created by ivans on 01/04/2026.
//

#ifndef SALAMANDER_PASS_NODE_H
#define SALAMANDER_PASS_NODE_H
#include <functional>
#include <string>
#include <vulkan/vulkan.h>
#include "render_graph/resource_access.h"

namespace Salamander::Renderer::RenderGraph::Internal {
    // Pass node is graph's knowledge about the pass
    struct ResourceReference {
        uint32_t resourceIndex;
        ResourceAccess access;
    };

    struct PassNode {
        std::string name{};
        VkPipelineStageFlagBits stageFlags{};
        std::vector<ResourceReference> resourceReferences{};
        std::function<void(VkCommandBuffer)> executeCallback{};
        bool culled = false;
    };
}


#endif //SALAMANDER_PASS_NODE_H