//
// Created by ivans on 15/08/2026.
//

#ifndef SALAMANDER_DEBUG_PANEL_REGISTRY_H
#define SALAMANDER_DEBUG_PANEL_REGISTRY_H

#include <memory>
#include <vector>
#include "debug_panel.h"

namespace Salamander::Renderer::Debug {
    class DebugPanelRegistry {
    public:
        template<typename PanelType, typename... PanelArgs>
        PanelType& addPanel(PanelArgs&&... panelArgs) {
            auto panel = std::make_unique<PanelType>(std::forward<PanelArgs>(panelArgs)...);
            PanelType& panelRef = *panel;
            m_panels.push_back(std::move(panel));
            return panelRef;
        }
        void draw(uint32_t frameIndex);
        void notifySwapchainRecreate() const;

        [[nodiscard]] bool& open() {
            return m_open;
        }

    private:
        std::vector<std::unique_ptr<IDebugPanel>> m_panels;
        bool m_open = true;
    };
}

#endif //SALAMANDER_DEBUG_PANEL_REGISTRY_H
