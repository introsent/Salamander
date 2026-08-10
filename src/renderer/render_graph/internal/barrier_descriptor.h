//
// Created by ivans on 08/08/2026.
//

#ifndef SALAMANDER_BARRIER_DESCRIPTOR_H
#define SALAMANDER_BARRIER_DESCRIPTOR_H

#include <optional>
#include <vulkan/vulkan.h>

namespace Salamander::Renderer::RenderGraph::Internal {
    struct ResourceState {
        VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 access = VK_ACCESS_2_NONE;
        std::optional<VkImageLayout> layout; // nullopt for buffers
    };

    struct BarrierDescriptor {
        uint32_t resourceIndex{};
        ResourceState src;
        ResourceState dst;
    };
}

#endif //SALAMANDER_BARRIER_DESCRIPTOR_H