//
// Created by ivans on 26/11/2025.
//

#ifndef SALAMANDER_TINYGLTF_LOADER_H
#define SALAMANDER_TINYGLTF_LOADER_H


#include "loaders/gltf_loader.h"

namespace Salamander::Resources::Loaders {
    class TinyGLTFLoader final : public GLTFLoader {
    public:
        static bool LoadFromFile(const std::string &path, GLTFModel &outModel);
    };
}


#endif //SALAMANDER_TINYGLTF_LOADER_H
