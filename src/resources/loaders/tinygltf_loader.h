#pragma once
#include "loaders/gltf_loader.h"

namespace Salamander::Resources::Loaders {
    class TinyGLTFLoader final : public GLTFLoader {
    public:
        static bool LoadFromFile(const std::string &path, GLTFModel &outModel);
    };
}
