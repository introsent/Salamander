//
// Created by ivans on 11/15/2025.
//

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include "assimp_loader.h"

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

    outModel = GLTFModel{};
    for (unsigned int meshInx = 0 ; meshInx < scene->mNumMeshes ; meshInx++) {
        const aiMesh* mesh = scene->mMeshes[meshInx];

        const aiVector3D zero3D (0.0f, 0.0f, 0.0f);
        for (unsigned int vertexInx = 0 ; vertexInx < mesh->mNumVertices ; vertexInx++)
        {
            Vertex vertex = {};
            const aiVector3D* pos = &(mesh->mVertices[vertexInx]);
            const aiVector3D* normal = &(mesh->mNormals[vertexInx]);
            const aiVector3D* tangent = &(mesh->mTangents[vertexInx]);
            const aiVector3D* texCoord = mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][vertexInx])
                                                                            : &zero3D;

            vertex.pos = glm::vec3(pos->x, pos->y, pos->z);
            vertex.normal = glm::vec3(normal->x, normal->y, normal->z);
            vertex.tangent = glm::vec4(tangent->x, tangent->y, tangent->z, 1);
            vertex.texCoord = glm::vec2(texCoord->x, texCoord->y);

            // Fill vertices vector
            outModel.vertices.push_back(vertex);
        }

        for (unsigned int faceInx = 0; faceInx < mesh->mNumFaces; faceInx++)
        {
            const aiFace& face = mesh->mFaces[faceInx];

            if (face.mNumIndices != 3)
            {
                std::cerr << "Number of indices must be 3" << std::endl;
            }

            // Fill indices vector
            outModel.indices.push_back(face.mIndices[0]);
            outModel.indices.push_back(face.mIndices[1]);
            outModel.indices.push_back(face.mIndices[2]);
        }

        GLTFPrimitive primitive {};
        primitive.materialIndex = mesh->mMaterialIndex;
        outModel.primitives.push_back(primitive);
    }


    //for (unsigned int textureInx = 0; textureInx < scene->mNumTextures; textureInx++)
    //{
    //    const aiTexture* texture = scene->mTextures[textureInx];
    //    outModel.textures.push_back ( {  .uri =  texture->mFilename.C_Str() });
    //}


    // We're done. Everything will be cleaned up by the importer destructor
    return true;

}