//
// Created by ivans on 11/15/2025.
//

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/postprocess.h>     // Post processing flags
#include "assimp_loader.h"
#include "shared/scene_data.h"
#include <config.h>
#include <iostream>

bool AssimpLoader::LoadFromFile(const std::string& path, GLTFModel& outModel)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path,
  aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);

    if (nullptr == scene) {
        std::cout << "AssimpLoader::LoadFromFile: failed to load scene" << std::endl;
        return false;
    }

    aiVector3D position, scale;
    aiQuaternion rotation;
    scene->mRootNode->mTransformation.Decompose(scale, rotation, position);
    globalScale = glm::vec3(scale.x, scale.y, scale.z);
    outModel = GLTFModel{};
    size_t vertexOffset = 0;
    size_t indexOffset  = 0;
    for (unsigned int meshInx = 0 ; meshInx < scene->mNumMeshes ; meshInx++)
    {
        const aiMesh* mesh = scene->mMeshes[meshInx];

        aiNode* meshNode = FindMeshNode(scene, meshInx, scene->mRootNode);
        glm::mat4 globalTransform = GetNodeGlobalTransform(meshNode);

        // Extract scale
        const glm::vec3 meshScale = {
            glm::length(glm::vec3(globalTransform[0])),
            glm::length(glm::vec3(globalTransform[1])),
            glm::length(glm::vec3(globalTransform[2]))
        };


        ProcessMesh(mesh, outModel, vertexOffset, indexOffset, meshScale);
    }


    for (unsigned int materialInx = 0; materialInx < scene->mNumMaterials; materialInx++) {
        aiMaterial* aiMaterial = scene->mMaterials[materialInx];

        GLTFMaterial material{};
        material.baseColorTexture = -1;
        material.metallicRoughnessTexture = -1;
        material.normalTexture = -1;

        // Albedo
        if (aiMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texturePath;
            aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);

            std::string texPath = texturePath.C_Str();
            material.baseColorTexture = outModel.textures.size();
            outModel.textures.push_back({ texPath });
        }

        // Normal
        if (aiMaterial->GetTextureCount(aiTextureType_NORMALS) > 0)
        {
            aiString texturePath;

            if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS)
            {
                std::string texPath = texturePath.C_Str();
                material.normalTexture = outModel.textures.size();
                outModel.textures.push_back({ texPath });
            }
        }

        // Metallic / Roughness
        if (aiMaterial->GetTextureCount(aiTextureType_METALNESS) > 0) {
            aiString texturePath;
            aiMaterial->GetTexture(aiTextureType_METALNESS, 0, &texturePath);

            std::string texPath = texturePath.C_Str();
            material.metallicRoughnessTexture = outModel.textures.size();
            outModel.textures.push_back({ texPath });
        }

        outModel.materials.push_back(material);
    }

    // We're done. Everything will be cleaned up by the importer destructor
    return true;

}

void AssimpLoader::ProcessMesh(const aiMesh* mesh, GLTFModel& outModel,
                            size_t& vertexOffset, size_t& indexOffset,
                            const glm::vec3& meshScale)
{
        GLTFPrimitive primitive{};

        primitive.vertexOffset = static_cast<uint32_t>(vertexOffset);
        primitive.indexOffset  = static_cast<uint32_t>(indexOffset);
        primitive.materialIndex = mesh->mMaterialIndex;

        // Load vertices
        const aiVector3D zero3D(0.f, 0.f, 0.f);

        size_t vertexCount = mesh->mNumVertices;
        outModel.vertices.reserve(outModel.vertices.size() + vertexCount);

        for (size_t i = 0; i < vertexCount; i++)
        {
            Vertex vertex{};

            const aiVector3D& position  = mesh->mVertices[i];
            const aiVector3D& normal = mesh->HasNormals()       ? mesh->mNormals[i]        : zero3D;
            const aiVector3D& tangent = mesh->HasTangentsAndBitangents()
                                        ? mesh->mTangents[i] : zero3D;
            const aiVector3D& bitangent = mesh->HasTangentsAndBitangents() ? mesh->mBitangents[i] : zero3D;
            const aiVector3D& uv   = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : zero3D;

            glm::vec3 T(tangent.x, tangent.y, tangent.z);
            glm::vec3 B(bitangent.x, bitangent.y, bitangent.z);
            glm::vec3 N(normal.x, normal.y, normal.z);

            vertex.pos      = glm::vec3(position.x,  position.y,  position.z) * meshScale;
            vertex.normal   = N;
            vertex.texCoord = glm::vec2(uv.x , -uv.y); // -Y for Vulkan

            float w = (glm::dot(glm::cross(T, B), N) < 0) ? -1.0f : 1.0f;
            vertex.tangent  = glm::vec4(tangent.x, tangent.y, tangent.z, w);

            outModel.vertices.push_back(vertex);
        }

        // Load indices
        size_t meshIndexCount = 0;
        outModel.indices.reserve(outModel.indices.size() + mesh->mNumFaces * 3);

        for (unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            const aiFace& face = mesh->mFaces[f];

            if (face.mNumIndices != 3)
            {
                std::cerr << "[AssimpLoader] Warning: non-triangular face encountered; skipping." << std::endl;
                continue;
            }

            outModel.indices.push_back(face.mIndices[0] + vertexOffset);
            outModel.indices.push_back(face.mIndices[1] + vertexOffset);
            outModel.indices.push_back(face.mIndices[2] + vertexOffset);

            meshIndexCount += 3;
        }

        primitive.vertexCount = static_cast<uint32_t>(vertexCount);
        primitive.indexCount  = static_cast<uint32_t>(meshIndexCount);

        // Store final primitive entry
        outModel.primitives.push_back(primitive);

        // Update global offsets
        vertexOffset += vertexCount;
        indexOffset  += meshIndexCount;
}

glm::mat4 AssimpLoader::GetNodeGlobalTransform(const aiNode* node)
{
    glm::mat4 transform = AiMatToGlm(node->mTransformation);
    const aiNode* parent = node->mParent;

    while (parent)
    {
        transform = AiMatToGlm(parent->mTransformation) * transform;
        parent = parent->mParent;
    }

    return transform;
}

aiNode* AssimpLoader::FindMeshNode(const aiScene* scene, unsigned meshIndex, aiNode* node)
{
    for (unsigned i = 0; i < node->mNumMeshes; ++i)
    {
        if (node->mMeshes[i] == meshIndex)
        {
            return node;
        }
    }

    for (unsigned i = 0; i < node->mNumChildren; ++i)
    {
        if (auto* found = FindMeshNode(scene, meshIndex, node->mChildren[i]))
        {
            return found;
        }
    }

    return nullptr;
}