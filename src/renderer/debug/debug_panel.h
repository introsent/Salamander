//
// Created by ivans on 15/08/2026.
//

#ifndef SALAMANDER_DEBUG_PANEL_H
#define SALAMANDER_DEBUG_PANEL_H
#include <cstdint>

namespace Salamander::Renderer::Debug {
    class IDebugPanel {
    public:
        virtual ~IDebugPanel() = default;

        [[nodiscard]] virtual const char* name() const = 0;

        virtual void draw(uint32_t frameIndex) = 0;

        virtual void onSwapchainRecreate() {}

        [[nodiscard]] bool& visible() {
            return m_visible;
        }

    protected:
        bool m_visible = true;
    };
}
#endif //SALAMANDER_DEBUG_PANEL_H
