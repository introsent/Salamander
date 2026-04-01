//
// Created by ivans on 28/03/2026.
//

#ifndef SALAMANDER_RENDER_RESOURCE_HANDLES_H
#define SALAMANDER_RENDER_RESOURCE_HANDLES_H

namespace Salamander::Renderer::RenderGraph {
    // indices to graph resource table
    struct RenderTextureHandle {
        uint32_t index = UINT32_MAX;
        [[nodiscard]] bool isValid() const { return index != UINT32_MAX; }
    };
    struct RenderBufferHandle {
        uint32_t index = UINT32_MAX;
        [[nodiscard]] bool isValid() const { return index != UINT32_MAX; }
    };
}
#endif //SALAMANDER_RENDER_RESOURCE_HANDLES_H