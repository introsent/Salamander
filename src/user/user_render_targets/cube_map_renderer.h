#pragma once

#include <memory>
#include <array>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "buffer_manager.h"
#include "pipeline.h"
#include "texture_manager.h"
#include "descriptors/descriptor_set_layout.h"
#include "shared/scene_data.h"
#include "deletion_queue.h"
#include "context.h"
#include "user_descriptor_managers/main_descriptor_manager.h"

struct CubeMapPushConstants {
    uint64_t vertexBufferAddress;  // For vertex pulling
    glm::mat4 viewProj;            // View-projection matrix
    uint32_t faceIndex;            // Current face being rendered
};

class CubeMapRenderer {
public:
    void initialize(Context* context, BufferManager* bufferManager, TextureManager* textureManager);

    struct CubeMap {
        ManagedTexture texture;
        std::array<VkImageView, 6> faceViews;
        VkImageView cubemapView;  // View for entire cubemap
    };

    CubeMap createCubeMap(uint32_t size, VkFormat format) const;
    void renderEquirectToCube(VkCommandBuffer cmd,
                              const ManagedTexture& equirectTexture,
                              const CubeMap& cubeMap) const;

    CubeMap createDiffuseIrradianceMap(VkCommandBuffer cmd, const CubeMap& environmentMap, uint32_t size);

private:
    void createPipelines();
    void createCubeFaceViews(CubeMap& cubeMap) const;
    void createCubeVertexData();

    void createDiffuseIrradiancePipeline();

    Context* m_context;
    BufferManager* m_bufferManager;
    TextureManager* m_textureManager;

    std::unique_ptr<DescriptorSetLayout> m_descriptorLayout;
    std::unique_ptr<MainDescriptorManager> m_descriptorManager;
    std::unique_ptr<Pipeline> m_pipeline;

    std::unique_ptr<Pipeline> m_diffuseIrradiancePipeline;
    std::unique_ptr<DescriptorSetLayout> m_diffuseIrradianceDescriptorLayout;
    std::unique_ptr<MainDescriptorManager> m_diffuseIrradianceDescriptorManager;

    ManagedBuffer m_cubeVertexBuffer;
    uint64_t m_vertexBufferAddress = 0;
    VkSampler m_equirectSampler;
};