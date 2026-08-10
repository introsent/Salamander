//
// Created by ivans on 09/08/2026.
//

#include "render_graph_debug_panel.h"
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <algorithm>
#include <ranges>

#include "textures/texture.h"

namespace Salamander::Renderer::Debug {

    namespace {
        constexpr int kPassIdOffset = 0;
        constexpr int kResourceIdOffset = 100000;
        constexpr int kPassOutPinOffset = 200000;
        constexpr int kPassInPinOffset = 300000;
        constexpr int kResInPinOffset = 400000;
        constexpr int kResOutPinOffset = 500000;
        constexpr float kColumnWidth = 220.0f;
        constexpr float kPassY = 0.0f;
        constexpr float kResourceY = 160.0f;
    }

    RenderGraphDebugPanel::RenderGraphDebugPanel(VkDevice device) : m_device(device) {
        ax::NodeEditor::Config config;
        config.SettingsFile = "render_graph_debug.json"; // persists node layout across runs
        m_editorContext = ax::NodeEditor::CreateEditor(&config);

        ImGui::GetIO().ConfigDebugHighlightIdConflicts = false;

        constexpr VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        };
        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_debugSampler);
    }

    RenderGraphDebugPanel::~RenderGraphDebugPanel() {
        invalidateCache();
        if (m_debugSampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_device, m_debugSampler, nullptr);
        }
        if (m_editorContext) {
            ax::NodeEditor::DestroyEditor(m_editorContext);
        }
    }

    void RenderGraphDebugPanel::invalidateCache() {
        for (const auto &descriptorSet: m_descriptorCache | std::views::values) {
            ImGui_ImplVulkan_RemoveTexture(descriptorSet);
        }
        m_descriptorCache.clear();
    }

    void RenderGraphDebugPanel::draw(const RenderGraph::Graph& graph, const uint32_t frameIndex) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);

        const float fullWidth = ImGui::GetIO().DisplaySize.x;
        ImGui::SetNextWindowSize(ImVec2(fullWidth, m_lastHeight), ImGuiCond_Always);
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;

        if (ImGui::Begin("Render Graph", nullptr, flags)) {
            drawTree(graph);
            ImGui::Separator();
            drawInspector(graph, frameIndex);
            m_lastHeight = ImGui::GetWindowSize().y; // remember height for next frame
        }
        ImGui::End();
    }

    void RenderGraphDebugPanel::drawTree(const RenderGraph::Graph& graph) {
        ax::NodeEditor::SetCurrentEditor(m_editorContext);
        ax::NodeEditor::Begin("RenderGraphCanvas", ImVec2(0, 400));

        const auto passes = graph.getDebugPasses();
        const auto resources = graph.getDebugResources();
        const auto& order = graph.getExecutionOrder();

        std::unordered_map<int, int> rankByPass;
        for (size_t r = 0; r < order.size(); ++r) rankByPass[order[r]] = static_cast<int>(r);

        for (int passIndex = 0; passIndex < static_cast<int>(passes.size()); ++passIndex) {
            const auto& pass = passes[passIndex];
            const int nodeId = kPassIdOffset + passIndex;
            const int rank = rankByPass.contains(passIndex) ? rankByPass[passIndex] : static_cast<int>(passes.size());

            if (!m_positionedNodes.contains(nodeId)) {
                ax::NodeEditor::SetNodePosition(nodeId, ImVec2(static_cast<float>(rank) * kColumnWidth, kPassY));
                m_positionedNodes.insert(nodeId);
            }

            ax::NodeEditor::BeginNode(nodeId);
            ImGui::TextUnformatted(pass.name.c_str());
            if (pass.culled) {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(culled)");
            }
            ax::NodeEditor::BeginPin(kPassInPinOffset + passIndex, ax::NodeEditor::PinKind::Input);
            ImGui::Text("in");
            ax::NodeEditor::EndPin();
            ImGui::SameLine();
            ax::NodeEditor::BeginPin(kPassOutPinOffset + passIndex, ax::NodeEditor::PinKind::Output);
            ImGui::Text("out");
            ax::NodeEditor::EndPin();
            ax::NodeEditor::EndNode();
        }

        for (uint32_t resourceIndex = 0; resourceIndex < resources.size(); ++resourceIndex) {
            const auto&[name, isBuffer] = resources[resourceIndex];
            const int nodeId = kResourceIdOffset + static_cast<int>(resourceIndex);

            int rank = 0;
            for (int passIndex = 0; passIndex < static_cast<int>(passes.size()); ++passIndex) {
                if (const auto& writes = passes[passIndex].writtenResourceIndices;
                    std::ranges::find(writes, resourceIndex) != writes.end())
                {
                    rank = rankByPass.contains(passIndex) ? rankByPass[passIndex] : 0;
                    break;
                }
            }

            if (!m_positionedNodes.contains(nodeId)) {
                ax::NodeEditor::SetNodePosition(nodeId, ImVec2(static_cast<float>(rank) * kColumnWidth + kColumnWidth * 0.5f, kResourceY));
                m_positionedNodes.insert(nodeId);
            }

            ax::NodeEditor::BeginNode(nodeId);
            ImGui::TextColored(isBuffer ? ImVec4(0.7f, 0.9f, 0.4f, 1) : ImVec4(0.4f, 0.7f, 0.9f, 1),
                                "%s", name.c_str());
            ax::NodeEditor::BeginPin(kResInPinOffset + resourceIndex, ax::NodeEditor::PinKind::Input);
            ImGui::Text("in");
            ax::NodeEditor::EndPin();
            ImGui::SameLine();
            ax::NodeEditor::BeginPin(kResOutPinOffset + resourceIndex, ax::NodeEditor::PinKind::Output);
            ImGui::Text("out");
            ax::NodeEditor::EndPin();
            ax::NodeEditor::EndNode();
        }

        int linkId = 600000;
        for (int passIndex = 0; passIndex < static_cast<int>(passes.size()); ++passIndex) {
            for (const uint32_t resourceIndex : passes[passIndex].writtenResourceIndices) {
                ax::NodeEditor::Link(linkId++, kPassOutPinOffset + passIndex, kResInPinOffset + resourceIndex);
            }
            for (const uint32_t resourceIndex : passes[passIndex].readResourceIndices) {
                ax::NodeEditor::Link(linkId++, kResOutPinOffset + resourceIndex, kPassInPinOffset + passIndex);
            }
       }

        if (const int selectedCount = ax::NodeEditor::GetSelectedObjectCount(); selectedCount > 0)
        {
            std::vector<ax::NodeEditor::NodeId> selected(static_cast<size_t>(selectedCount));
            const int actual = ax::NodeEditor::GetSelectedNodes(selected.data(), selectedCount);
            m_selectedPassIndex.reset();
            for (int i = 0; i < actual; ++i) {
                if (const auto raw = static_cast<int>(selected[i].Get());
                    raw >= kPassIdOffset && raw < kResourceIdOffset) {
                    m_selectedPassIndex = raw - kPassIdOffset;
                    break;
                    }
            }
        } else {
            m_selectedPassIndex.reset();
        }

        ax::NodeEditor::End();
        ax::NodeEditor::SetCurrentEditor(nullptr);
    }

    void RenderGraphDebugPanel::drawInspector(const RenderGraph::Graph& graph, uint32_t frameIndex) {
        if (!m_selectedPassIndex) {
            ImGui::TextDisabled("Select a pass node to inspect its outputs.");
            return;
        }

        const auto passes = graph.getDebugPasses();
        const auto resources = graph.getDebugResources();
        const int passIndex = *m_selectedPassIndex;
        if (passIndex < 0 || passIndex >= static_cast<int>(passes.size())) return;

        const auto& pass = passes[passIndex];
        ImGui::Text("Pass: %s%s", pass.name.c_str(), pass.culled ? " (culled)" : "");

        if (pass.writtenResourceIndices.empty()) {
            ImGui::TextDisabled("This pass writes no graph-tracked resources.");
            return;
        }

        std::vector<uint32_t> uniqueWritten(pass.writtenResourceIndices.begin(), pass.writtenResourceIndices.end());
        std::ranges::sort(uniqueWritten);
        uniqueWritten.erase(std::ranges::unique(uniqueWritten).begin(), uniqueWritten.end());

        for (const uint32_t resourceIndex : uniqueWritten) {
            const auto&[name, isBuffer] = resources[resourceIndex];
            ImGui::PushID(static_cast<int>(resourceIndex));
            ImGui::Text("%s", name.c_str());

            if (isBuffer) {
                ImGui::TextDisabled("(buffer => no image preview)");
                ImGui::PopID();
                continue;
            }

            const auto* texture = graph.getPhysicalTexture(resourceIndex, frameIndex);
            if (!texture) {
                ImGui::TextDisabled("(not bound this frame)");
                ImGui::PopID();
                continue;
            }

            const glm::ivec2 size = texture->getImage()->size();
            if (size.x <= 4 && size.y <= 4) {
                ImGui::TextDisabled("(%dx%d => too small to preview meaningfully)", size.x, size.y);
                ImGui::PopID();
                continue;
            }

            const VkImageLayout layout = texture->getImage()->currentLayout();
            VkDescriptorSet ds = getOrCreateDebugDescriptor(
                resourceIndex, frameIndex, texture->getDescriptorInfo().imageView, layout);

            constexpr float kPreviewWidth = 256.0f;
            const float aspect = size.y > 0 ? static_cast<float>(size.x) / static_cast<float>(size.y) : 1.0f;
            ImGui::Image(reinterpret_cast<ImTextureID>(ds), ImVec2(kPreviewWidth, kPreviewWidth / aspect));

            ImGui::PopID();
        }
    }

    VkDescriptorSet RenderGraphDebugPanel::getOrCreateDebugDescriptor(const uint32_t resourceIndex,
        const uint32_t frameIndex, VkImageView view, const VkImageLayout layout)
    {
        const uint64_t key = (static_cast<uint64_t>(resourceIndex) << 32) | frameIndex;
        if (const auto it = m_descriptorCache.find(key); it != m_descriptorCache.end()) {
            return it->second;
        }
        VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(m_debugSampler, view, layout);
        m_descriptorCache[key] = descriptorSet;
        return descriptorSet;
    }
}
