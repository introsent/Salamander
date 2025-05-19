#pragma once
#include "target/render_target.h"
#include "pipeline.h"
#include "descriptors/descriptor_set_layout.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "uniform_buffer.h"
#include "user_descriptor_managers/main_descriptor_manager.h"
#include "data_structures.h"
#include "config.h"
#include "ssbo_buffer.h"
#include "loaders/gltf_loader.h"

class MainSceneTarget : public RenderTarget {
public:
    void initialize(const SharedResources& shared) override;
    void render(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
    void recreateSwapChain() override;
    void cleanup() override;

    void updateUniformBuffers() const override;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    const std::string MODEL_PATH = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/Sponza.gltf";

private:
    // G-Buffer attachments
    ManagedTexture m_albedoTexture;
    ManagedTexture m_normalTexture;
    ManagedTexture m_paramTexture;

    VkSampler      m_gBufferSampler = VK_NULL_HANDLE;

    void createGBufferSampler();
    void createGBufferAttachments();

    std::vector<GLTFPrimitiveData> m_primitives;
    std::vector<ManagedTexture> m_modelTextures;
    std::vector<ManagedTexture> m_materialTextures;
    std::vector<std::string> m_defaultMaterialKeys;

    void updateLightingDescriptors();

    void createLightingPipeline();  // Renamed from createPipeline
    void createDepthPrepassPipeline();
    void createGBufferPipeline();
    void createRenderingResources();
    void createDescriptors();
    void loadModel(const std::string& path);

    uint32_t createDefaultMaterialTexture(float metallicFactor, float roughnessFactor);

    void createBuffers();
    // Updates

    // Pipelines
    std::unique_ptr<Pipeline> m_lightingPipeline;  // Renamed from m_pipeline
    std::unique_ptr<Pipeline> m_depthPrepassPipeline;
    std::unique_ptr<Pipeline> m_gBufferPipeline;

    // Descriptor layouts and managers
    std::unique_ptr<DescriptorSetLayout> m_gBufferDescriptorLayout;  // For depth and G-buffer passes
    std::unique_ptr<MainDescriptorManager> m_gBufferDescriptorManager;
    std::unique_ptr<DescriptorSetLayout> m_lightingDescriptorLayout;  // For lighting pass
    std::unique_ptr<MainDescriptorManager> m_lightingDescriptorManager;

    // Vertices and indices of initial model
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    // Vertex buffer (vertex pulling)
    SSBOBuffer m_ssboBuffer;
    VkDeviceAddress m_deviceAddress {};

    // Model
    IndexBuffer m_indexBuffer;
    std::vector<UniformBuffer> m_uniformBuffers;

    // Framedata
    struct FrameData {
        VkDescriptorBufferInfo bufferInfo;
        std::vector<VkDescriptorImageInfo> textureImageInfos;
        std::vector<VkDescriptorImageInfo> materialImageInfos;
    };
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_frameData;
};