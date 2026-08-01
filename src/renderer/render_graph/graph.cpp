//
// Created by ivans on 28/03/2026.
//

#include "graph.h"

#include <iostream>

#include "helpers.h"

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

    void Graph::cullDeadPasses() {
        int defaultNonCulledPassIndex = -1;
        std::visit([&](auto& resource) {
            if (resource.writtenByPasses.size() > 1) {
                std::cerr << "GRAPH ERROR: Multiple passes are writing to the set Output" << std::endl;
            }
            if (!resource.writtenByPasses.empty()) {
                defaultNonCulledPassIndex = resource.writtenByPasses[0];
            }
        }, m_resources[m_resourceIndex.at(m_output)]);

        if (defaultNonCulledPassIndex == -1) {
            std::cerr << "GRAPH ERROR: Output is not used by any pass" << std::endl;
        }
        m_passes[defaultNonCulledPassIndex].culled = false;

        traversePass(defaultNonCulledPassIndex);

        // every pass by default is set to culled=true
    }

    void Graph::configureExecutionSequence() {
        const int amountOfPasses = static_cast<int>(m_passes.size());
        std::vector<std::vector<uint32_t>> orderedPassForKhan(amountOfPasses);

        // Setup orderedPassForKhan
        for (const auto &pass : m_passes) {
            for (const auto& [resourceIndex, access] : pass.resourceReferences) {
                std::visit([&](auto& resource) {
                    if (isWrite(access)) {
                        if (resource.writtenByPasses.size() > 1) {
                            std::cerr << "[Render Graph]: Multiple passes are writing to the " << resource.name << std::endl;
                        }
                        if (const int writtenByPassIndex = resource.writtenByPasses[0];
                            m_passes[writtenByPassIndex].culled == false)
                        {
                            std::vector<uint32_t> readByPasses = resource.readByPasses;
                            std::erase_if(readByPasses,
                                [this](const uint32_t readByPassIndex)
                                {
                                    return m_passes[readByPassIndex].culled;
                                });
                            orderedPassForKhan[writtenByPassIndex].insert(
                                orderedPassForKhan[writtenByPassIndex].end(),
                                readByPasses.begin(), readByPasses.end());
                        }
                    }
                }, m_resources[resourceIndex]);
            }
        }

        m_orderedPassIndices = Helpers::topologicalSort( orderedPassForKhan, amountOfPasses);
        std::erase_if(m_orderedPassIndices, [this](int index) { return m_passes[index].culled; });
    }

    void Graph::setOutput(const std::string &name) {
        m_output = name;
    }

    void Graph::logExecutionOrder() const {
        std::cout << "[Render Graph] Execution order: ";
        for (const int passIndex : m_orderedPassIndices) {
            std::cout << m_passes[passIndex].name << "(" << passIndex << ") ";
        }
        std::cout << std::endl;
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

    void Graph::traversePass(int passIndex) {
        for (const auto& [resourceIndex, access] : m_passes[passIndex].resourceReferences) {
            if (isWrite(access)) {
                continue;
            }
            std::visit([&](auto& resource) {
                if (resource.writtenByPasses.empty()) {
                    return;
                }
                for (const int writtenByPassInx : resource.writtenByPasses) {
                    if (!m_passes[writtenByPassInx].culled) {
                        continue; // already visited
                    }
                    m_passes[writtenByPassInx].culled = false;
                    traversePass(writtenByPassInx);
                }
            }, m_resources[resourceIndex]);
        }
    }
};

