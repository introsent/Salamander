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


        for (unsigned int vertexInx = 0 ; vertexInx < mesh->mNumVertices ; vertexInx++)
        {
            Vertex vertex = {};
            const aiVector3D* pos = &(mesh->mVertices[vertexInx]);
            vertex.pos = glm::vec3(pos->x, pos->y, pos->z);

            outModel.vertices.push_back(vertex);
        }
    }


    //DoTheSceneProcessing( scene);

    // We're done. Everything will be cleaned up by the importer destructor
    return true;

}
