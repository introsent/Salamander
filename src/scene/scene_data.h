#pragma once
#include "components/material.h"
#include "components/transform.h"
#include "resources/buffers/index_buffer.h"
#include "resources/buffers/ssbo_buffer.h"
#include "renderer/frame/frame_data.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <array>

namespace Salamander {
    class Texture;
    namespace Scene {
        /// MainSceneData contains all the scene-wide data: geometry, materials, textures
        struct MainSceneData {
            // Geometry resources
            std::vector<GLTFPrimitiveData> primitives;
            SSBOBuffer vertexBuffer;
            uint64_t vertexBufferAddress;
            IndexBuffer indexBuffer;

            // Material resources
            std::vector<Texture*> modelTextures;    // albedo textures
            std::vector<Texture*> materialTextures; // metal/roughness textures
            std::vector<Texture*> normalTextures;   // normal maps

            // Scene bounds
            AABB sceneAABB;

            // Scene transformation
            glm::vec3 modelScale{1.0f, 1.0f, 1.0f};

            // Per-frame descriptor data
            struct FrameData {
                VkDescriptorBufferInfo bufferInfo;
                std::vector<VkDescriptorImageInfo> textureImageInfos;
                std::vector<VkDescriptorImageInfo> materialImageInfos;
                std::vector<VkDescriptorImageInfo> normalImageInfos;
                VkDescriptorBufferInfo omniLightBufferInfo;
                VkDescriptorBufferInfo cameraExposureBufferInfo;
                VkDescriptorBufferInfo directionalLightBufferInfo;
            };
            std::array<FrameData, Renderer::MAX_FRAMES_IN_FLIGHT> frameData;

            float deltaTime = 0.0f;
        };
    }
}
