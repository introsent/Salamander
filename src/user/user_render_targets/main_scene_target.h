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

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    const std::string MODEL_PATH = std::string(SOURCE_RESOURCE_DIR) + "/models/viking_room.obj";
    const std::string TEXTURE_PATH = std::string(SOURCE_RESOURCE_DIR) + "/textures/viking_room.png";
private:
    struct PrimitiveData {
        SSBOBuffer vertexBuffer;
        IndexBuffer indexBuffer;
        VkDeviceAddress vertexBufferAddress;
        uint32_t indexCount;
        int materialIndex;
    };


    void createPipeline();
    void createRenderingResources();
    void createDescriptors();
    void loadModel(const std::string& path);
    void createBuffers();

    // Updates
    void updateUniformBuffers() const;

    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<DescriptorSetLayout> m_descriptorLayout;
    std::unique_ptr<MainDescriptorManager> m_descriptorManager;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    SSBOBuffer m_ssboBuffer;
    VkDeviceAddress m_deviceAddress {};


    IndexBuffer m_indexBuffer;
    std::vector<UniformBuffer> m_uniformBuffers;
    ManagedTexture m_texture;
};
