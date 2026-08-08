//
// Created by ivans on 28/03/2026.
//

#ifndef SALAMANDER_RESOURCE_ACCESS_H
#define SALAMANDER_RESOURCE_ACCESS_H
#include "pipeline/compute_pipeline.h"
#include <map>

namespace Salamander::Renderer::RenderGraph {
    enum class ResourceAccess {
        ColorAttachmentWrite,
        DepthAttachmentWrite,
        AttachmentInput,
        TextureSampled,
        StorageRead,
        StorageWrite,
        TransferSrc,
        TransferDst
    };
}

#endif //SALAMANDER_RESOURCE_ACCESS_H