#pragma once
#include <glm/glm.hpp>

namespace Salamander::Scene {
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 cameraPosition;
    };

    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
    };
}
