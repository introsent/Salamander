//
// Created by ivans on 15/08/2026.
//

#ifndef SALAMANDER_SCENE_DEBUG_PANEL_H
#define SALAMANDER_SCENE_DEBUG_PANEL_H
#include <functional>

#include "debug_panel.h"

namespace Salamander::Renderer::Debug {
    class SceneDebugPanel final : public IDebugPanel {
    public:
        explicit SceneDebugPanel(std::function<void(uint32_t)> drawFn) : m_drawFn(std::move(drawFn)) {}

        [[nodiscard]] const char * name() const override {
            return "Scene";
        }
        void draw(const uint32_t frameIndex) override {
            m_drawFn(frameIndex);
        }
    private:
        std::function<void(uint32_t)> m_drawFn;
    };
}

#endif //SALAMANDER_SCENE_DEBUG_PANEL_H
