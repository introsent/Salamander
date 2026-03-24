#include "cube_map_renderer.h"

#include <array>
#include <stdexcept>
#include "config.h"
#include "deletion_queue.h"
#include "graphics/image_transition_manager.h"
#include "descriptors/descriptor_set_layout_builder.h"
#include "textures/texture_manager.h"

namespace Salamander::Renderer::Targets {

    void CubeMapRenderer::initialize(const Frame::RenderContext &ctx) {
        m_ctx = &ctx;
        createPipelines();
        createCubeVertexData();
    }

    void CubeMapRenderer::createCubeVertexData() {
        const std::vector<glm::vec3> cubeVertices = {
            // +X
            {1,-1,-1},{1,-1,1},{1,1,1},  {1,1,1},{1,1,-1},{1,-1,-1},
            // -X
            {-1,-1,1},{-1,-1,-1},{-1,1,-1},  {-1,1,-1},{-1,1,1},{-1,-1,1},
            // +Y
            {-1,1,-1},{1,1,-1},{1,1,1},  {1,1,1},{-1,1,1},{-1,1,-1},
            // -Y
            {-1,-1,1},{1,-1,1},{1,-1,-1},  {1,-1,-1},{-1,-1,-1},{-1,-1,1},
            // +Z
            {-1,-1,1},{-1,1,1},{1,1,1},  {1,1,1},{1,-1,1},{-1,-1,1},
            // -Z
            {1,-1,-1},{1,1,-1},{-1,1,-1},  {-1,1,-1},{-1,-1,-1},{1,-1,-1}
        };

        const VkDeviceSize bufferSize = sizeof(glm::vec3) * cubeVertices.size();
        auto &bm = m_ctx->bufferManager();

        Resources::Buffers::ManagedBuffer staging = bm.createBuffer(
            bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY
        );

        void *data;
        vmaMapMemory(bm.allocator(), staging.allocation, &data);
        memcpy(data, cubeVertices.data(), static_cast<size_t>(bufferSize));
        vmaUnmapMemory(bm.allocator(), staging.allocation);

        m_cubeVertexBuffer = bm.createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT  |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );

        bm.copyBuffer(staging.buffer, m_cubeVertexBuffer.buffer, bufferSize);

        VkBufferDeviceAddressInfo addressInfo{
            .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = m_cubeVertexBuffer.buffer
        };
        m_vertexBufferAddress = vkGetBufferDeviceAddress(
            m_ctx->context().device(), &addressInfo
        );
    }

    CubeMapRenderer::CubeMap CubeMapRenderer::createCubeMap(
        uint32_t size, VkFormat format) const
    {
        CubeMap cubeMap{};
        cubeMap.texture = &m_ctx->textureManager().createCubeTexture(
            size, format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
        createCubeFaceViews(cubeMap);
        return cubeMap;
    }

    void CubeMapRenderer::createCubeFaceViews(CubeMap &cubeMap) const {
        VkDevice device = m_ctx->context().device();

        static int imageViewCount = 0;
        static int cubeMapViewCount = 0;

        for (uint32_t face = 0; face < 6; ++face) {
            const VkImageViewCreateInfo viewInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = cubeMap.texture->getImage()->handle(),
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = cubeMap.texture->getImage()->format(),
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = face,
                    .layerCount = 1
                }
            };

            if (vkCreateImageView(device, &viewInfo, nullptr,
                                  &cubeMap.faceViews[face]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create cube map face view");

            DeletionQueue::get().pushFunction(
                "CubeFaceView_" + std::to_string(++imageViewCount),
                [device, view = cubeMap.faceViews[face]]() {
                    vkDestroyImageView(device, view, nullptr);
                }
            );
        }

        const VkImageViewCreateInfo cubeViewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = cubeMap.texture->getImage()->handle(),
            .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
            .format = cubeMap.texture->getImage()->format(),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 6
            }
        };

        if (vkCreateImageView(device, &cubeViewInfo, nullptr,
                              &cubeMap.cubemapView) != VK_SUCCESS)
            throw std::runtime_error("Failed to create cube map view");

        DeletionQueue::get().pushFunction(
            "CubeMapView_" + std::to_string(++cubeMapViewCount),
            [device, view = cubeMap.cubemapView]() {
                vkDestroyImageView(device, view, nullptr);
            }
        );
    }

    void CubeMapRenderer::renderEquirectToCube(
        VkCommandBuffer cmd,
        const Resources::Textures::Texture *equirectTexture,
        const CubeMap &cubeMap) const
    {
        Graphics::ImageTransitionManager::transitionImageLayout(
            cmd, equirectTexture->getImage()->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 1
        );

        VkDescriptorImageInfo imageInfo{
            .sampler = m_equirectSampler,
            .imageView = equirectTexture->getDescriptorInfo().imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        std::vector<Graphics::Descriptors::MainDescriptorManager::DescriptorUpdateInfo> updates = {{
            .binding = 0,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = &imageInfo,
            .descriptorCount = 1,
            .isImage = true
        }};
        m_descriptorManager->updateDescriptorSet(0, updates);

        Graphics::ImageTransitionManager::transitionImageLayout(
            cmd, cubeMap.texture->getImage()->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 6, 1
        );

        const glm::mat4 proj = [] {
            glm::mat4 p = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
            p[1][1] *= -1;
            return p;
        }();

        const std::array<glm::mat4, 6> viewMatrices = {
            glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0)),
        };

        const glm::ivec2 extent = cubeMap.texture->getImage()->size();
        const auto uw = static_cast<uint32_t>(extent.x);
        const auto uh = static_cast<uint32_t>(extent.y);

        for (uint32_t face = 0; face < 6; ++face) {
            const VkRenderingAttachmentInfo colorAttachment{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = cubeMap.faceViews[face],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {{0.0f, 0.0f, 0.0f, 1.0f}}
            };
            const VkRenderingInfo renderInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = {{0,0},{uw,uh}},
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachment
            };

            vkCmdBeginRendering(cmd, &renderInfo);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());

            const VkViewport viewport{0,0,static_cast<float>(uw),static_cast<float>(uh),0,1};
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            const VkRect2D scissor{{0,0},{uw,uh}};
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pipeline->layout(), 0, 1,
                &m_descriptorManager->getDescriptorSets()[0], 0, nullptr);

            const CubeMapPushConstants pc{
                .vertexBufferAddress = m_vertexBufferAddress,
                .viewProj = proj * viewMatrices[face],
                .faceIndex = face
            };
            vkCmdPushConstants(cmd, m_pipeline->layout(),
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(pc), &pc);

            vkCmdDraw(cmd, 36, 1, 0, 0);
            vkCmdEndRendering(cmd);
        }

        Graphics::ImageTransitionManager::transitionImageLayout(
            cmd, cubeMap.texture->getImage()->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6, 1
        );
    }

    CubeMapRenderer::CubeMap CubeMapRenderer::createDiffuseIrradianceMap(
        VkCommandBuffer cmd, const CubeMap &environmentMap, uint32_t size)
    {
        CubeMap irradianceMap = createCubeMap(size, VK_FORMAT_R16G16B16A16_SFLOAT);

        if (!m_diffuseIrradiancePipeline)
            createDiffuseIrradiancePipeline();

        VkDescriptorImageInfo imageInfo{
            .sampler = m_equirectSampler,
            .imageView = environmentMap.cubemapView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        std::vector<Graphics::Descriptors::MainDescriptorManager::DescriptorUpdateInfo> updates = {{
            .binding = 0,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = &imageInfo,
            .descriptorCount = 1,
            .isImage = true
        }};
        m_diffuseIrradianceDescriptorManager->updateDescriptorSet(0, updates);

        Graphics::ImageTransitionManager::transitionImageLayout(
            cmd, irradianceMap.texture->getImage()->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 6, 1
        );

        const glm::mat4 proj = [] {
            glm::mat4 p = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
            p[1][1] *= -1;
            return p;
        }();

        const std::array<glm::mat4, 6> viewMatrices = {
            glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0)),
        };

        for (uint32_t face = 0; face < 6; ++face) {
            const VkRenderingAttachmentInfo colorAttachment{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = irradianceMap.faceViews[face],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {{0.0f, 0.0f, 0.0f, 1.0f}}
            };
            const VkRenderingInfo renderInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = {{0,0},{size,size}},
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachment
            };

            vkCmdBeginRendering(cmd, &renderInfo);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              m_diffuseIrradiancePipeline->handle());

            const VkViewport viewport{0,0,static_cast<float>(size),static_cast<float>(size),0,1};
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            const VkRect2D scissor{{0,0},{size,size}};
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_diffuseIrradiancePipeline->layout(), 0, 1,
                &m_diffuseIrradianceDescriptorManager->getDescriptorSets()[0], 0, nullptr);

            const CubeMapPushConstants pc{
                .vertexBufferAddress = m_vertexBufferAddress,
                .viewProj = proj * viewMatrices[face],
                .faceIndex = face
            };
            vkCmdPushConstants(cmd, m_diffuseIrradiancePipeline->layout(),
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(pc), &pc);

            vkCmdDraw(cmd, 36, 1, 0, 0);
            vkCmdEndRendering(cmd);
        }

        Graphics::ImageTransitionManager::transitionImageLayout(
            cmd, irradianceMap.texture->getImage()->handle(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6, 1
        );

        return irradianceMap;
    }

    void CubeMapRenderer::createPipelines() {
        VkDevice device = m_ctx->context().device();

        Graphics::Descriptors::DescriptorSetLayoutBuilder layoutBuilder(device);
        m_descriptorLayout = layoutBuilder
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
        };
        m_descriptorManager = std::make_unique<Graphics::Descriptors::MainDescriptorManager>(
            device, m_descriptorLayout->handle(), poolSizes, 1
        );

        // Sampler
        const VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .anisotropyEnable = VK_FALSE,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE
        };
        if (vkCreateSampler(device, &samplerInfo, nullptr, &m_equirectSampler) != VK_SUCCESS)
            throw std::runtime_error("Failed to create equirect sampler");

        DeletionQueue::get().pushFunction("EquirectSampler",
            [device, sampler = m_equirectSampler]() {
                vkDestroySampler(device, sampler, nullptr);
            }
        );

        static constexpr std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };

        const VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        const VkPipelineRenderingCreateInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorFormat
        };

        const VkPipelineColorBlendAttachmentState blendAttachment{
            .blendEnable    = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };

        Graphics::Pipeline::PipelineConfig config{};
        config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/equirect_to_cube_vert.spv";
        config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/equirect_to_cube_frag.spv";
        config.inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        config.viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                    .viewportCount = 1, .scissorCount = 1};
        config.rasterizer= {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                    .polygonMode = VK_POLYGON_MODE_FILL,
                                    .cullMode = VK_CULL_MODE_NONE,
                                    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                    .lineWidth = 1.0f};
        config.multisampling= {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
        config.depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                    .depthTestEnable = VK_FALSE, .depthWriteEnable = VK_FALSE};
        config.colorBlendAttachments = {blendAttachment};
        config.colorBlending = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                    .attachmentCount = 1, .pAttachments = &blendAttachment};
        config.dynamicState= {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                                    .pDynamicStates = dynamicStates.data()};
        config.rendering = renderingInfo;

        constexpr VkPushConstantRange pushConstant
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(CubeMapPushConstants)
        };

        m_pipeline = std::make_unique<Graphics::Pipeline::Pipeline>(
            &m_ctx->context(), m_descriptorLayout->handle(), config, pushConstant
        );
    }

    void CubeMapRenderer::createDiffuseIrradiancePipeline() {
        VkDevice device = m_ctx->context().device();

        Graphics::Descriptors::DescriptorSetLayoutBuilder layoutBuilder(device);
        m_diffuseIrradianceDescriptorLayout = layoutBuilder
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
        };
        m_diffuseIrradianceDescriptorManager =
            std::make_unique<Graphics::Descriptors::MainDescriptorManager>(
                device, m_diffuseIrradianceDescriptorLayout->handle(), poolSizes, 1
            );

        static constexpr std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };

        const VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        const VkPipelineRenderingCreateInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorFormat
        };

        const VkPipelineColorBlendAttachmentState blendAttachment{
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };

        Graphics::Pipeline::PipelineConfig config{};
        config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/equirect_to_cube_vert.spv";
        config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/diffuse_irradiance_frag.spv";
        config.inputAssembly= {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        config.viewportState= {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                    .viewportCount = 1, .scissorCount = 1};
        config.rasterizer= {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                    .polygonMode = VK_POLYGON_MODE_FILL,
                                    .cullMode = VK_CULL_MODE_NONE,
                                    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                    .lineWidth = 1.0f};
        config.multisampling= {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
        config.depthStencil= {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                    .depthTestEnable = VK_FALSE, .depthWriteEnable = VK_FALSE};
        config.colorBlendAttachments = {blendAttachment};
        config.colorBlending= {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                    .attachmentCount = 1, .pAttachments = &blendAttachment};
        config.dynamicState= {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                                    .pDynamicStates = dynamicStates.data()};
        config.rendering = renderingInfo;

        const VkPushConstantRange pushConstant{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(CubeMapPushConstants)
        };

        m_diffuseIrradiancePipeline = std::make_unique<Graphics::Pipeline::Pipeline>(
            &m_ctx->context(), m_diffuseIrradianceDescriptorLayout->handle(), config, pushConstant
        );
    }
}
