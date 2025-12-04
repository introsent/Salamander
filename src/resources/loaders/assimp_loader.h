//
// Created by ivans on 11/15/2025.
//

#ifndef SALAMANDER_ASSIMP_LOADER_H
#define SALAMANDER_ASSIMP_LOADER_H

#include <assimp/scene.h>           // Output data structure
#include "loaders/gltf_loader.h"
#include "string"

class AssimpLoader : GLTFLoader
{
public:

    static bool LoadFromFile(const std::string& path, GLTFModel& outModel);
private:
    static void ProcessMesh(const aiMesh* mesh, GLTFModel& outModel, size_t& vertexOffset, size_t& indexOffset);
    static aiNode* FindMeshNode(const aiScene* scene, unsigned meshIndex, aiNode* node);
};


#endif //SALAMANDER_ASSIMP_LOADER_H