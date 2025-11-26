// tinygltf_loader.h
#pragma once
#include "loaders/gltf_loader.h"

class TinyGLTFLoader : public GLTFLoader
{
public:
    static bool LoadFromFile(const std::string& path, GLTFModel& outModel);
};