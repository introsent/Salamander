//
// Created by ivans on 28/03/2026.
//

#ifndef SALAMANDER_RENDER_RESOURCE_HANDLES_H
#define SALAMANDER_RENDER_RESOURCE_HANDLES_H

namespace Salamander::Renderer::RenderGraph {
    // indices to graph resource table
    struct RenderTextureHandle {
        int index;
    };
    struct RenderBufferHandle {
        int index;
    };
}
#endif //SALAMANDER_RENDER_RESOURCE_HANDLES_H