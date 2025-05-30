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
    VkSampler hdrSampler = VK_NULL_HANDLE;

    // Frame data
    struct FrameData {
        VkDescriptorBufferInfo bufferInfo;
        VkDescriptorBufferInfo omniLightBufferInfo;
        VkDescriptorBufferInfo cameraExposureBufferInfo;
    };
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frameData;
};

struct PassDependencies {
    // Inputs/Outputs
    ManagedTexture* depthTexture;
    ManagedTexture* albedoTexture;
    ManagedTexture* normalTexture;
    ManagedTexture* paramTexture;
    ManagedTexture* hdrTexture;

    // Layout tracking
    std::array<VkImageLayout, MAX_FRAMES_IN_FLIGHT> depthLayouts;

    void transitionDepth(VkCommandBuffer cmd, uint32_t frameIndex,
                         VkImageLayout newLayout);
};