//
// Created by ivans on 15/08/2026.
//

#include "debug_panel_registry.h"
#include <imgui.h>

#include "debug_panel.h"

namespace Salamander::Renderer::Debug {
    void DebugPanelRegistry::draw(uint32_t frameIndex) {
        if (m_open == false) {
            return;
        }

        constexpr ImVec2 windowPos { 0.f, 0.f };
        constexpr ImVec2 windowSize { 1280.f, 720.f };

        ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

        if (constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;
            ImGui::Begin("Salamander Debug", &m_open, flags))
        {
            if (ImGui::BeginTabBar("Debug Panel Tabs", ImGuiTabBarFlags_Reorderable)) {
                for (const auto& panel : m_panels) {
                    if (panel->visible() == false) {
                        continue;
                    }
                    if (ImGui::BeginTabItem(panel->name())) {
                        panel->draw(frameIndex);
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void DebugPanelRegistry::notifySwapchainRecreate() const {
        for (auto& panel : m_panels) {
            panel->onSwapchainRecreate();
        }
    }
}
