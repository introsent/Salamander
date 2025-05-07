#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#define GLM_FORCE_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


struct Vertex {
    glm::vec4 pos;
    glm::vec4 color;
    glm::vec4 texCoord;


    ///USE FOR RAW VERTEX MANAGEMENT
    //static VkVertexInputBindingDescription getBindingDescription() {
    //    VkVertexInputBindingDescription bindingDescription{};
    //    bindingDescription.binding = 0;
    //    bindingDescription.stride = sizeof(Vertex);
    //    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//
    //    return bindingDescription;
    //}
//
//
    //static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
    //    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
//
    //    attributeDescriptions[0].binding = 0;
    //    attributeDescriptions[0].location = 0;
    //    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    //    attributeDescriptions[0].offset = offsetof(Vertex, pos);
//
    //    attributeDescriptions[1].binding = 0;
    //    attributeDescriptions[1].location = 1;
    //    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    //    attributeDescriptions[1].offset = offsetof(Vertex, color);
//
    //    attributeDescriptions[2].binding = 0;
    //    attributeDescriptions[2].location = 2;
    //    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    //    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
//
    //    return attributeDescriptions;
    //}
    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec4>()(vertex.pos) ^ (hash<glm::vec4>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec4>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
