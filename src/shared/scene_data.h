#pragma once
#include "data_structures.h"
#include "index_buffer.h"
#include "ssbo_buffer.h"

struct MainSceneGlobalData {
    // Resources
    std::vector<Texture*> modelTextures;
    std::vector<Texture*> materialTextures;
    std::vector<Texture*> normalTextures;
    std::vector<GLTFPrimitiveData> primitives;
    SSBOBuffer vertexBuffer;
    uint64_t vertexBufferAddress;
    IndexBuffer indexBuffer;

    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
    };
    AABB sceneAABB;

    // Frame data
    struct FrameData {
        VkDescriptorBufferInfo bufferInfo;
        std::vector<VkDescriptorImageInfo> textureImageInfos;
        std::vector<VkDescriptorImageInfo> materialImageInfos;
        std::vector<VkDescriptorImageInfo> normalImageInfos;
        VkDescriptorBufferInfo omniLightBufferInfo;
        VkDescriptorBufferInfo cameraExposureBufferInfo;
        VkDescriptorBufferInfo directionalLightBufferInfo;
    };
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frameData;
};

inline glm::vec3 globalScale{1.0f, 1.0f, 1.0f};

struct PassDependencies {
    // Per-frame textures
    std::array<Texture*, MAX_FRAMES_IN_FLIGHT> albedoTextures{};
    std::array<Texture*, MAX_FRAMES_IN_FLIGHT> normalTextures{};
    std::array<Texture*, MAX_FRAMES_IN_FLIGHT> paramTextures{};
    std::array<Texture*, MAX_FRAMES_IN_FLIGHT> hdrTextures{};
    std::array<ManagedBuffer, MAX_FRAMES_IN_FLIGHT> histogramBuffers{};

    // Static Textures
    Texture* equirectTexture{};
    Texture* cubeMap{};
    Texture* irradianceMap{};
    Texture* shadowMap{};

    void transitionDepth(VkCommandBuffer cmd, uint32_t frameIndex,
                         VkImageLayout newLayout);
};