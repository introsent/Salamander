// gltf_loader.cpp
#include "gltf_loader.h"
#include <unordered_map>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include "glm/gtc/type_ptr.hpp"

namespace {
    struct VertexHash {
        size_t operator()(const GLTFVertex& v) const {
            size_t seed = 0;
            auto combine = [&seed](auto val) {
                seed ^= std::hash<decltype(val)>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

            combine(v.pos.x); combine(v.pos.y); combine(v.pos.z);
            combine(v.texCoord.x); combine(v.texCoord.y);
            // Add more fields if needed
            return seed;
        }
    };

    bool loadGLTFModel(tinygltf::Model& model, const std::string& path) {
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool success = path.ends_with(".glb") ?
            loader.LoadBinaryFromFile(&model, &err, &warn, path) :
            loader.LoadASCIIFromFile(&model, &err, &warn, path);

        if (!warn.empty()) { /* handle warning */ }
        if (!err.empty()) { /* handle error */ }

        return success;
    }
}

bool GLTFLoader::loadFromFile(const std::string& path, GLTFModel& outModel) {
    tinygltf::Model model;
    if (!loadGLTFModel(model, path)) return false;

    // Process materials
    for (const auto& mat : model.materials) {
        GLTFMaterial material;
        material.baseColorTexture = mat.pbrMetallicRoughness.baseColorTexture.index;
        outModel.materials.push_back(material);
    }

    // Process meshes
    for (const auto& mesh : model.meshes) {
        for (const auto& prim : mesh.primitives) {
            GLTFPrimitive primitive;
            primitive.materialIndex = prim.material;

            // Get accessors
            const auto& posAccessor = model.accessors[prim.attributes.at("POSITION")];
            const auto& texAccessor = model.accessors[prim.attributes.at("TEXCOORD_0")];
            const auto& normalAccessor = model.accessors[prim.attributes.at("NORMAL")];

            // Get buffer views
            const auto& posView = model.bufferViews[posAccessor.bufferView];
            const auto& texView = model.bufferViews[texAccessor.bufferView];
            const auto& normalView = model.bufferViews[normalAccessor.bufferView];

            // Get raw data pointers
            const float* positions = reinterpret_cast<const float*>(
                &model.buffers[posView.buffer].data[posView.byteOffset + posAccessor.byteOffset]);
            const float* texCoords = reinterpret_cast<const float*>(
                &model.buffers[texView.buffer].data[texView.byteOffset + texAccessor.byteOffset]);
            const float* normals = reinterpret_cast<const float*>(
                &model.buffers[normalView.buffer].data[normalView.byteOffset + normalAccessor.byteOffset]);

            // Process indices
            const auto& indexAccessor = model.accessors[prim.indices];
            const auto& indexView = model.bufferViews[indexAccessor.bufferView];
            const void* indexData = &model.buffers[indexView.buffer].data[indexView.byteOffset + indexAccessor.byteOffset];

            std::unordered_map<GLTFVertex, uint32_t, VertexHash> uniqueVertices;

            for (size_t i = 0; i < indexAccessor.count; ++i) {
                uint32_t index = 0;
                switch (indexAccessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        index = static_cast<const uint16_t*>(indexData)[i];
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        index = static_cast<const uint32_t*>(indexData)[i];
                        break;
                    default:
                        throw std::runtime_error("Unsupported index type");
                }

                GLTFVertex vertex{};
                vertex.pos = glm::make_vec3(&positions[3 * index]);
                vertex.texCoord = glm::make_vec2(&texCoords[2 * index]);
                vertex.normal = glm::make_vec3(&normals[3 * index]);
                vertex.color = glm::vec4(1.0f); // Default color

                if (!uniqueVertices.contains(vertex)) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(primitive.vertices.size());
                    primitive.vertices.push_back(vertex);
                }
                primitive.indices.push_back(uniqueVertices[vertex]);
            }

            outModel.primitives.push_back(primitive);
        }
    }

    return true;
}