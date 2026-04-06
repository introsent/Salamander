//
// Created by ivans on 28/03/2026.
//

#ifndef SALAMANDER_RENDER_GRAPH_H
#define SALAMANDER_RENDER_GRAPH_H
#include <string>

#include "pass_builder.h"
#include "render_resource_handles.h"
#include "resource_description_types.h"
#include "internal/pass_node.h"
#include "internal/resource_nodes.h"
#include "pipeline/compute_pipeline.h"

namespace Salamander::Renderer::RenderGraph {
    class Graph {
    public:
        Graph();

        PassBuilder addPass(const std::string& name, VkPipelineStageFlagBits stageFlag);
        [[nodiscard]] const Internal::PassNode& getPass(uint32_t index) const { return m_passes[index]; }
        [[nodiscard]] uint32_t resourceCount() const { return static_cast<uint32_t>( m_resources.size()); }
        [[nodiscard]] uint32_t passCount() const { return static_cast<uint32_t>( m_passes.size()); }

        RenderTextureHandle addTexture(const std::string& name, const ImageAttachmentDescription& description);
        RenderBufferHandle addBuffer(const std::string& name, const BufferAttachmentDescription& description);

        void buildEdges();
        void cullDeadPasses();

        void setOutput(const std::string& name);

    private:
        static bool isWrite(ResourceAccess access);

        void traversePass(int passIndex);

        std::string m_output;

        std::vector<Internal::PassNode> m_passes;
        std::unordered_map<std::string, uint32_t> m_passIndex;

        std::vector<Internal::ResourceNode> m_resources;
        std::unordered_map<std::string, uint32_t> m_resourceIndex;
    };
}
#endif //SALAMANDER_RENDER_GRAPH_H