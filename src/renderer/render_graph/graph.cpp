//
// Created by ivans on 28/03/2026.
//

#include "graph.h"

#include <iostream>

namespace Salamander::Renderer::RenderGraph {
    Graph::Graph() {
        m_passes.reserve(16); // prevent reallocation
    }

    PassBuilder Graph::addPass(const std::string &name, VkPipelineStageFlagBits stageFlag)
    {
        assert(!m_passIndex.contains(name) && "Pass with this name already exists");

        const auto index = static_cast<uint32_t>(m_passes.size());
        m_passes.emplace_back(name, stageFlag);
        m_passIndex[name] = index;

        return PassBuilder(m_passes.back(), m_resources, m_resourceIndex);
    }

    RenderTextureHandle Graph::addTexture(const std::string &name, const ImageAttachmentDescription &description)
    {
        assert(!m_resourceIndex.contains(name) && "Texture with this name already exists");

        const auto index = static_cast<uint32_t>(m_resources.size());
        m_resources.emplace_back(Internal::ImageResourceNode{
            {
                name,
                {},
                {},
                UINT32_MAX,
                0
            },
            description,
            0,
            VK_IMAGE_LAYOUT_UNDEFINED,
        });
        m_resourceIndex[name] = index;

        return RenderTextureHandle{ index };
    }

    RenderBufferHandle Graph::addBuffer(const std::string &name, const BufferAttachmentDescription &description)
    {
        assert(!m_resourceIndex.contains(name) && "Buffer with this name already exists");

        const auto index = static_cast<uint32_t>(m_resources.size());
        m_resources.emplace_back(Internal::BufferResourceNode{
            {
                name,
                {},
                {},
                UINT32_MAX,
                0
            },
            description,
            0,
        });
        m_resourceIndex[name] = index;

        return RenderBufferHandle{ index };
    }

    void Graph::buildEdges() {
        for (const auto &pass : m_passes) {
            for (const auto& [resourceIndex, access] : pass.resourceReferences) {
                std::visit([&](auto& resource){
                    if (isWrite(access)) {
                        resource.writtenByPasses.push_back(m_passIndex[pass.name]);
                    }
                    else {
                        resource.readByPasses.push_back(m_passIndex[pass.name]);
                    }
                }, m_resources[resourceIndex]);
            }
        }
    }

    bool Graph::isWrite(const ResourceAccess access) {
        if (access == ResourceAccess::ColorAttachmentWrite ||
            access == ResourceAccess::DepthAttachmentWrite ||
            access == ResourceAccess::StorageWrite ||
            access == ResourceAccess::TransferDst) {
            return true;
        }
        return false;
    }
};

