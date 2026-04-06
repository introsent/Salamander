//
// Created by ivans on 28/03/2026.
//

#ifndef SALAMANDER_PASS_BUILDER_H
#define SALAMANDER_PASS_BUILDER_H
#include <string>

#include "resource_access.h"
#include "resource_description_types.h"
#include "internal/pass_node.h"
#include "internal/resource_nodes.h"

namespace Salamander::Renderer::RenderGraph {
    // Used as a setup to let declare what pass does
    class PassBuilder {
    public:
        explicit PassBuilder(Internal::PassNode& node,
                            std::vector<Internal::ResourceNode>& resources,
                            std::unordered_map<std::string, uint32_t>& resourceIndex);
        void add(const std::string& name, ResourceAccess access) const;
        void addExecuteCallback(const std::function<void(VkCommandBuffer)>& callback) const;

    private:
        // builder holds a reference to pass node and resource nodes of the graph
        Internal::PassNode& m_node;
        std::vector<Internal::ResourceNode>& m_resources;
        std::unordered_map<std::string, uint32_t>& m_resourceIndex;
    };
}
#endif //SALAMANDER_PASS_BUILDER_H