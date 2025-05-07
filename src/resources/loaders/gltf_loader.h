// gltf_loader.h
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <tiny_gltf.h>

struct GLTFVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 tangent;
    glm::vec4 color;

    bool operator==(GLTFVertex const& other) const {
        return pos == other.pos
            && normal   == other.normal
            && texCoord == other.texCoord
            && tangent == other.tangent
            && color == other.color;
            // && … compare any other fields …
    }
};

struct GLTFPrimitive {
    std::vector<GLTFVertex> vertices;
    std::vector<uint32_t> indices;
    int materialIndex = -1;
};

struct GLTFMaterial {
    int baseColorTexture = -1;
    // Add more PBR properties as needed
};

struct GLTFModel {
    std::vector<GLTFPrimitive> primitives;
    std::vector<GLTFMaterial> materials;
    // Add more model-level data as needed
};

class GLTFLoader {
public:
    static bool loadFromFile(const std::string& path, GLTFModel& outModel);
};