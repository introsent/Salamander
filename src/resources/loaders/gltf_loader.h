//
// Created by ivans on 26/11/2025.
//

#ifndef SALAMANDER_GLTF_LOADER_H
#define SALAMANDER_GLTF_LOADER_H

#include <vector>
#include <string>
#include "components/vertex.h"

namespace Salamander::Resources::Loaders {
    class GLTFLoader {
    public:
        struct GLTFPrimitive {
            uint32_t vertexOffset;
            uint32_t vertexCount;
            uint32_t indexOffset;
            uint32_t indexCount;
            uint32_t materialIndex;
        };

        struct GLTFMaterial {
            int baseColorTexture = -1;
            int metallicRoughnessTexture = -1;
            int normalTexture = -1;
            float metallicFactor = 1.0f;
            float roughnessFactor = 1.0f;
        };

        struct GLTFTexture {
            std::string uri;
        };

        struct GLTFModel {
            std::vector<Salamander::Scene::Vertex> vertices;
            std::vector<uint32_t> indices;
            std::vector<GLTFPrimitive> primitives;
            std::vector<GLTFMaterial> materials;
            std::vector<GLTFTexture> textures;
        };
    };
}
#endif //SALAMANDER_GLTF_LOADER_H