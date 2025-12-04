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
    static void ProcessMesh(const aiMesh* mesh, GLTFModel& outModel,
                            size_t& vertexOffset, size_t& indexOffset,
                            const glm::vec3& meshScale);

    static constexpr glm::mat4 AiMatToGlm(const aiMatrix4x4& m)
    {
        return {
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        };
    }

    static glm::mat4 GetNodeGlobalTransform(const aiNode* node);
    static aiNode* FindMeshNode(const aiScene* scene, unsigned meshIndex, aiNode* node);
};


#endif //SALAMANDER_ASSIMP_LOADER_H