//
// Created by ivans on 20/03/2026.
//

#ifndef SALAMANDER_TRANSFORM_H
#define SALAMANDER_TRANSFORM_H


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


#endif //SALAMANDER_TRANSFORM_H
