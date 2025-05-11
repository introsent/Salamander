// gltf_loader.cpp
#include "gltf_loader.h"
#include <tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <iostream>
#include <tiny_gltf.h>

namespace {
    void ProcessPrimitive(
        const tinygltf::Model& model,
        const tinygltf::Primitive& primitive,
        GLTFModel& result,
        size_t& vertexOffset,
        size_t& indexOffset)
    {
        GLTFPrimitive primData;
        primData.vertexOffset = static_cast<uint32_t>(vertexOffset);
        primData.indexOffset = static_cast<uint32_t>(indexOffset);
        primData.materialIndex = primitive.material;


        // Get accessors
        const auto& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
        const auto& texAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];

        // Get buffer data
        const auto& posView = model.bufferViews[posAccessor.bufferView];
        const auto& texView = model.bufferViews[texAccessor.bufferView];

        const float* positions = reinterpret_cast<const float*>(
            &model.buffers[posView.buffer].data[posView.byteOffset + posAccessor.byteOffset]);
        const float* texCoords = reinterpret_cast<const float*>(
            &model.buffers[texView.buffer].data[texView.byteOffset + texAccessor.byteOffset]);

        // Process vertices
        const size_t vertexCount = posAccessor.count;
        for (size_t i = 0; i < vertexCount; ++i) {
            Vertex v{};
            v.pos = glm::make_vec3(&positions[3 * i]);
            v.texCoord = glm::make_vec2(&texCoords[2 * i]);
           // v.texCoord.y = 1.0f - v.texCoord.y; // Flip Y
            v.color = glm::vec3(1.0f);
            result.vertices.push_back(v);
        }

        // Process indices
        const auto& indexAccessor = model.accessors[primitive.indices];
        const auto& indexView = model.bufferViews[indexAccessor.bufferView];
        const void* indexData = &model.buffers[indexView.buffer].data[indexView.byteOffset + indexAccessor.byteOffset];

        primData.indexCount = static_cast<uint32_t>(indexAccessor.count);
        primData.vertexCount = static_cast<uint32_t>(vertexCount);

        switch (indexAccessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                const uint16_t* src = static_cast<const uint16_t*>(indexData);
                for (size_t i = 0; i < indexAccessor.count; ++i) {
                    result.indices.push_back(src[i] + vertexOffset); // Add vertexOffset
                }
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                const uint32_t* src = static_cast<const uint32_t*>(indexData);
                for (size_t i = 0; i < indexAccessor.count; ++i) {
                    result.indices.push_back(src[i] + vertexOffset); // Add vertexOffset
                }
                break;
            }

            default:
                throw std::runtime_error("Unsupported index type");
        }

        result.primitives.push_back(primData);
        vertexOffset += vertexCount;
        indexOffset += indexAccessor.count;
    }
}

bool GLTFLoader::LoadFromFile(const std::string& path, GLTFModel& outModel) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool success = path.find(".glb") != std::string::npos ?
        loader.LoadBinaryFromFile(&model, &err, &warn, path) :
        loader.LoadASCIIFromFile(&model, &err, &warn, path);

    if (!success) return false;

    // Clear output model
    outModel = GLTFModel{};

    // Find the first node with a mesh and get its scale
    for (const auto& node : model.nodes) {
        if (node.mesh >= 0 && !node.scale.empty()) {
            // Update global scale with the node's scale
            globalScale = glm::vec3(
                static_cast<float>(node.scale[0]),
                static_cast<float>(node.scale[1]),
                static_cast<float>(node.scale[2])
            );
            break;  // Use the first mesh node's scale
        }
    }

    // Process materials
    for (const auto& mat : model.materials) {
        GLTFMaterial material;
        material.baseColorTexture = -1; // Default value for no texture
        if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            material.baseColorTexture = mat.pbrMetallicRoughness.baseColorTexture.index;
        }
        outModel.materials.push_back(material);
    }

    // Process meshes
    size_t vertexOffset = 0;
    size_t indexOffset = 0;
    for (const auto& mesh : model.meshes) {
        for (const auto& prim : mesh.primitives) {
            if (prim.mode != TINYGLTF_MODE_TRIANGLES) continue;
            ProcessPrimitive(model, prim, outModel, vertexOffset, indexOffset);
        }
    }

    // Process textures
    for (const auto& tex : model.textures) {
        const auto& image = model.images[tex.source];
        GLTFTexture texture;
        texture.uri = image.uri;
        outModel.textures.push_back(texture);
    }
    std::cout << "Total textures loaded: " << outModel.textures.size() << std::endl;

    return true;
}