#pragma once

namespace Salamander::Scene
{
    struct GLTFPrimitiveData {
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t materialIndex;
        uint32_t metalRoughTextureIndex;
        uint32_t normalTextureIndex;
    };

    struct RenderObject {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t textureID;
    };
}
