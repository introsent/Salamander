//
// Created by ivans on 28/03/2026.
//

#include "pass_builder.h"

#include <cassert>

namespace Salamander::Renderer::RenderGraph {
    PassBuilder::PassBuilder(Internal::PassNode &node,
                            std::vector<Internal::ResourceNode> &resources,
                            std::unordered_map<std::string, uint32_t>& resourceIndex) :
        m_node(node), m_resources(resources), m_resourceIndex(resourceIndex)
    {
    }

    void PassBuilder::add(const std::string &name, const ResourceAccess access) const {
        assert(m_resourceIndex.contains(name) && "Resource must be pre-registered");
        const uint32_t index = m_resourceIndex.at(name);
        m_node.resourceReferences.push_back({ index, access });
    }

    void PassBuilder::addExecuteCallback(const std::function<void(VkCommandBuffer)> &callback) const {
        m_node.executeCallback = callback;

    }
}

