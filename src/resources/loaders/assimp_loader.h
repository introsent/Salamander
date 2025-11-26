//
// Created by ivans on 11/15/2025.
//

#ifndef SALAMANDER_ASSIMP_LOADER_H
#define SALAMANDER_ASSIMP_LOADER_H

#include "loaders/gltf_loader.h"
#include "string"

class AssimpLoader : GLTFLoader
{
public:

    static bool LoadFromFile(const std::string& path, GLTFModel& outModel);
};


#endif //SALAMANDER_ASSIMP_LOADER_H