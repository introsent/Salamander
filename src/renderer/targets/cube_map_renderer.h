#pragma once

#include <memory>
#include <array>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "renderer/frame/render_context.h"
#include "pipeline/pipeline.h"
#include "descriptors/descriptor_set_layout.h"
#include "descriptors/managers/main_descriptor_manager.h"
#include "resources/buffers/buffer_manager.h"
#include "resources/textures/texture.h"

namespace Salamander::Renderer::Targets {

    struct CubeMapPushConstants {
        uint64_t vertexBufferAddress;
        glm::mat4 viewProj;
        uint32_t faceIndex;
    };

    class CubeMapRenderer {
    public:
        void initialize(const Frame::RenderContext &ctx);

        struct CubeMap {
            Resources::Textures::Texture* texture = nullptr;
            std::array<VkImageView, 6> faceViews{};
            VkImageView cubemapView{};
        };

        [[nodiscard]] CubeMap createCubeMap(uint32_t size, VkFormat format) const;

        void renderEquirectToCube(VkCommandBuffer cmd,
                                  const Resources::Textures::Texture *equirectTexture,
                                  const CubeMap &cubeMap) const;

        CubeMap createDiffuseIrradianceMap(VkCommandBuffer cmd,
                                           const CubeMap &environmentMap,
                                           uint32_t size);

    private:
        void createPipelines();
        void createCubeFaceViews(CubeMap &cubeMap) const;
        void createCubeVertexData();
        void createDiffuseIrradiancePipeline();

        const Frame::RenderContext *m_ctx = nullptr;

        std::unique_ptr<Graphics::Descriptors::DescriptorSetLayout> m_descriptorLayout;
        std::unique_ptr<Graphics::Descriptors::MainDescriptorManager> m_descriptorManager;
        std::unique_ptr<Graphics::Pipeline::Pipeline> m_pipeline;

        std::unique_ptr<Graphics::Pipeline::Pipeline> m_diffuseIrradiancePipeline;
        std::unique_ptr<Graphics::Descriptors::DescriptorSetLayout> m_diffuseIrradianceDescriptorLayout;
        std::unique_ptr<Graphics::Descriptors::MainDescriptorManager> m_diffuseIrradianceDescriptorManager;

        Resources::Buffers::ManagedBuffer m_cubeVertexBuffer = {};
        uint64_t m_vertexBufferAddress = 0;
        VkSampler m_equirectSampler = VK_NULL_HANDLE;
    };
}