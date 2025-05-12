// gltf_loader.h
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>

#include "data_structures.h"

struct GLTFPrimitive {
    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t indexOffset;
    uint32_t indexCount;
    uint32_t materialIndex;
};

struct GLTFMaterial {
    int   baseColorTexture   = -1;
    int   metallicRoughnessTexture = -1;
    float metallicFactor     = 1.0f;
    float roughnessFactor    = 1.0f;
};

struct GLTFTexture {
    std::string uri;
};

inline glm::vec3 globalScale{1.0f, 1.0f, 1.0f};

struct GLTFModel {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GLTFPrimitive> primitives;
    std::vector<GLTFMaterial> materials;
    std::vector<GLTFTexture> textures;
};

class GLTFLoader {
public:
    static bool LoadFromFile(const std::string& path, GLTFModel& outModel);
};