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
    struct ImageResourceNode {
        std::string name;
        ImageAttachmentDescription description;
        VkImageUsageFlags usageFlags;
        VkImageLayout currentLayout;
        std::vector<uint32_t> writtenByPasses;
        std::vector<uint32_t> readByPasses;
        uint32_t physicalIndex;
        uint32_t version;
    };

    struct BufferResourceNode {
        std::string name;
        BufferAttachmentDescription description;
        VkBufferUsageFlags usageFlags;
        std::vector<uint32_t> writtenByPasses;
        std::vector<uint32_t> readByPasses;
        uint32_t physicalIndex;
        uint32_t version;
    };

    using ResourceNode = std::variant<ImageResourceNode, BufferResourceNode>;
}

#endif //SALAMANDER_RESOURCE_NODES_H