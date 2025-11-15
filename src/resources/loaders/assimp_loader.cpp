//
// Created by ivans on 11/15/2025.
//

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include "assimp_loader.h"

#include <iostream>

bool AssimpLoader::LoadFromFile(const std::string& path)
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

    //DoTheSceneProcessing( scene);

    // We're done. Everything will be cleaned up by the importer destructor
    return true;

}
