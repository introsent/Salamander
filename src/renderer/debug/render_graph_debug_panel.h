//
// Created by ivans on 09/08/2026.
//

#ifndef SALAMANDER_RENDER_GRAPH_DEBUG_PANEL_H
#define SALAMANDER_RENDER_GRAPH_DEBUG_PANEL_H
#include <unordered_set>
#include <vulkan/vulkan_core.h>
#include <imgui_node_editor.h>
#include "render_graph/graph.h"



namespace Salamander::Renderer::Debug {
    class RenderGraphDebugPanel {
    public:
        explicit RenderGraphDebugPanel(VkDevice device);
        ~RenderGraphDebugPanel();

        RenderGraphDebugPanel(const RenderGraphDebugPanel&) = delete;
        RenderGraphDebugPanel& operator=(const RenderGraphDebugPanel&) = delete;

        // call once per frame, between ImGui::NewFrame() and ImGui::Render()
        void draw(const RenderGraph::Graph& graph, uint32_t frameIndex);

        // call after any swapchain/extent-relative texture recreation, before draw()
        void invalidateCache();
    private:
        void drawTree(const RenderGraph::Graph& graph);
        void drawInspector(const RenderGraph::Graph& graph, uint32_t frameIndex);
        VkDescriptorSet getOrCreateDebugDescriptor(uint32_t resourceIndex, uint32_t frameIndex,
                                                    VkImageView view, VkImageLayout layout);

        VkDevice m_device;
        ax::NodeEditor::EditorContext* m_editorContext = nullptr;
        VkSampler m_debugSampler = VK_NULL_HANDLE;

        std::unordered_map<uint64_t, VkDescriptorSet> m_descriptorCache;
        std::unordered_set<int> m_positionedNodes;
        std::optional<int> m_selectedPassIndex;

        float m_lastHeight = 700.0f;
    };
}
#endif //SALAMANDER_RENDER_GRAPH_DEBUG_PANEL_H
