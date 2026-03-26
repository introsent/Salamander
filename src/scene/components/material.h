//
// Created by ivans on 20/03/2026.
//

#ifndef SALAMANDER_MATERIAL_H
#define SALAMANDER_MATERIAL_H


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


#endif //SALAMANDER_MATERIAL_H