//
// Created by ivans on 01/04/2026.
//

#ifndef SALAMANDER_RESOURCE_NODES_H
#define SALAMANDER_RESOURCE_NODES_H
#include <string>
#include <variant>
#include <vector>
#include <vulkan/vulkan.h>

#include "render_graph/resource_description_types.h"

namespace Salamander::Renderer::RenderGraph::Internal {
    // Buffer and Image resource nodes are graph's knowledge about a resource
    struct BaseResourceNode {
        std::string name;
        std::vector<uint32_t> writtenByPasses;
        std::vector<uint32_t> readByPasses;
        uint32_t physicalIndex;
        uint32_t version;
    };

    struct ImageResourceNode : BaseResourceNode {
        ImageAttachmentDescription description;
        VkImageUsageFlags usageFlags;

        VkAccessFlags2 currentAccess = VK_ACCESS_2_NONE;
        VkPipelineStageFlags2 currentStage = VK_PIPELINE_STAGE_2_NONE;
        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct BufferResourceNode : BaseResourceNode {
        BufferAttachmentDescription description;
        VkBufferUsageFlags usageFlags;

        VkAccessFlags2 currentAccess = VK_ACCESS_2_NONE;
        VkPipelineStageFlags2 currentStage = VK_PIPELINE_STAGE_2_NONE;
    };

    using ResourceNode = std::variant<ImageResourceNode, BufferResourceNode>;
}

#endif //SALAMANDER_RESOURCE_NODES_H