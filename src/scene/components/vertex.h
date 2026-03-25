#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace Salamander::Scene {
    struct Vertex {
        glm::vec3 pos;
        glm::vec2 texCoord;
        glm::vec3 normal;
        glm::vec4 tangent;

        bool operator==(const Vertex& other) const {
            return pos == other.pos &&
                   texCoord == other.texCoord &&
                   normal == other.normal &&
                   tangent == other.tangent;
        }
    };
}

template<>
struct std::hash<Salamander::Scene::Vertex> {
    size_t operator()(const Salamander::Scene::Vertex& vertex) const noexcept {
        return ((hash<glm::vec3>()(vertex.pos) ^
                 (hash<glm::vec2>()(vertex.texCoord) << 1) ^
                 (hash<glm::vec3>()(vertex.normal) >> 1)) >> 1) ^
               (hash<glm::vec4>()(vertex.tangent) >> 1);
    }
};
