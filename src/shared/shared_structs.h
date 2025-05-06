#pragma once
#include <cstdint>

// 16‐byte alignment is guaranteed on most GPUs,
// but push constants themselves have offset/size rules at the pipeline‐layout level.
struct PushConstants {
    // This must match exactly GLSL definition:
    uint64_t vertexBufferAddress;
};