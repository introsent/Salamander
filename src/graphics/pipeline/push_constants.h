#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Salamander::Graphics::Pipeline {

    // 16-byte alignment is guaranteed on most GPUs,
    // but push constants themselves have offset/size rules at the pipeline-layout level.
    struct PushConstants {
        uint64_t vertexBufferAddress;
        uint32_t baseColorTextureIndex;      // index for albedo texture
        uint32_t metalRoughTextureIndex;     // index for material texture
        uint32_t normalTextureIndex;         // index for normal texture
        uint32_t textureCount;
        glm::vec3 modelScale;
    };

    struct TonePush {
        glm::vec2 screenSize;
    };

    struct ShadowPushConstants {
        uint64_t vertexBufferAddress;
        glm::vec3 modelScale;
        uint32_t baseColorTextureIndex;
    };

}
