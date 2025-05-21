#pragma once
#include <cstdint>

// 16‐byte alignment is guaranteed on most GPUs,
// but push constants themselves have offset/size rules at the pipeline‐layout level.
struct PushConstants {
    uint64_t vertexBufferAddress;
    uint32_t baseColorTextureIndex;      // Index for albedo texture
    uint32_t metalRoughTextureIndex;     // Index for material texture
    uint32_t normalTextureIndex;         // Index for normal texture
    uint32_t textureCount;
    glm::vec3 modelScale;
};

struct TonePush {
    glm::vec2 screenSize;
};