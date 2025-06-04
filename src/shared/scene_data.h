#pragma once
#include "data_structures.h"
#include "index_buffer.h"
#include "ssbo_buffer.h"
#include "texture_manager.h"

struct MainSceneGlobalData {
    // Resources
    std::vector<ManagedTexture> modelTextures;
    std::vector<ManagedTexture> materialTextures;
    std::vector<ManagedTexture> normalTextures;
    std::vector<GLTFPrimitiveData> primitives;
    SSBOBuffer ssboBuffer;
    uint64_t vertexBufferAddress;
    IndexBuffer indexBuffer;

    // Samplers
    VkSampler gBufferSampler = VK_NULL_HANDLE;
    VkSampler depthSampler = VK_NULL_HANDLE;
    VkSampler shadowDepthSampler = VK_NULL_HANDLE;
    VkSampler hdrSampler = VK_NULL_HANDLE;

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

struct PassDependencies {
    // Per-frame textures
    std::array<ManagedTexture*, MAX_FRAMES_IN_FLIGHT> depthTextures;
    std::array<ManagedTexture*, MAX_FRAMES_IN_FLIGHT> albedoTextures;
    std::array<ManagedTexture*, MAX_FRAMES_IN_FLIGHT> normalTextures;
    std::array<ManagedTexture*, MAX_FRAMES_IN_FLIGHT> paramTextures;
    std::array<ManagedTexture*, MAX_FRAMES_IN_FLIGHT> hdrTextures;

    // Static Textures
    ManagedTexture* equirectTexture;
    ManagedTexture* cubeMap;
    ManagedTexture* irradianceMap;
    ManagedTexture* shadowMap;

    // Layout tracking
    std::array<VkImageLayout, MAX_FRAMES_IN_FLIGHT> depthLayouts;
    std::array<ManagedTexture*, MAX_FRAMES_IN_FLIGHT> perFrameDepthTextures;

    void transitionDepth(VkCommandBuffer cmd, uint32_t frameIndex,
                         VkImageLayout newLayout);
};