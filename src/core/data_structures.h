#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#define GLM_FORCE_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <memory>
#include <glm/gtx/hash.hpp>

#include "command_buffer.h"
#include "texture_manager.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec4 tangent;

    bool operator==(const Vertex& other) const {
        return pos == other.pos && texCoord == other.texCoord && normal == other.normal && tangent == other.tangent;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec2>()(vertex.texCoord) << 1) ^ (hash<glm::vec3>()(vertex.normal) >> 1)) >> 1) ^ (hash<glm::vec4>()(vertex.tangent) >> 1);
        }
    };
}

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 cameraPosition;
};


struct RenderObject {
    uint32_t firstIndex;
    uint32_t indexCount;
    int32_t textureID;
};


struct GLTFPrimitiveData {
    uint32_t indexOffset;
    uint32_t indexCount;
    uint32_t materialIndex;
    uint32_t metalRoughTextureIndex;
    uint32_t normalTextureIndex;
};

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct Frame {
    std::unique_ptr<CommandBuffer> commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    ManagedTexture depthTexture;
};

struct PointLightData {
    glm::vec3 pointLightPosition;
    float pointLightIntensity;  // In lumens
    glm::vec3 pointLightColor;
    float pointLightRadius;     // Maximum radius of influence for optimization
};

inline struct DirectionalLightData {
    glm::vec3 directionalLightPosition; // well kind of
    glm::vec3 directionalLightDirection;
    glm::vec3 directionalLightColor;
    float directionalLightIntensity;
    glm::mat4 view;
    glm::mat4 projection;
} directionalLight { glm::vec3{0.f, 0.f, 0.f}, glm::vec3{0.f, -1.f, 0.f},
    glm::vec3{ 1.f, 1.f, 1.f}, 10000.f,
    glm::mat4{ 1.f }, glm::mat4{ 1.f } };

inline struct CameraExposure {
    float aperture;
    float shutterSpeed;
    float ISO;
    float ev100Override;
} camExpUBO{ 5.f, 1.0f/ 200.0f, 100.0f, -1.0f };
