//
// Created by ivans on 20/03/2026.
//

#ifndef SALAMANDER_LIGHTS_H
#define SALAMANDER_LIGHTS_H


#include <glm/glm.hpp>

namespace Salamander::Scene {
    struct PointLightData {
        glm::vec3 pointLightPosition;
        float pointLightIntensity;  // in lumens
        glm::vec3 pointLightColor;
        float pointLightRadius;
        int enabled;
    };

    struct DirectionalLightData {
        glm::vec3 directionalLightPosition; // light source position
        glm::vec3 directionalLightDirection;
        glm::vec3 directionalLightColor;
        float directionalLightIntensity;
        glm::mat4 view;
        glm::mat4 projection;
    };
}


#endif //SALAMANDER_LIGHTS_H
