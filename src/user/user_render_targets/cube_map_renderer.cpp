#include "cube_map_renderer.h"
#include "image_transition_manager.h"
#include <array>

#include "config.h"
#include "descriptors/descriptor_set_layout_builder.h"

void CubeMapRenderer::initialize(Context* context, BufferManager* bufferManager, TextureManager* textureManager) {

    m_context = context;
    m_bufferManager = bufferManager;
    m_textureManager = textureManager;
    createPipelines();
    createCubeVertexData();
}

void CubeMapRenderer::createCubeVertexData() {
    const std::vector<glm::vec3> cubeVertices = {
        // +X face
        {1, -1, -1}, {1, -1,  1}, {1,  1,  1},
        {1,  1,  1}, {1,  1, -1}, {1, -1, -1},

        // -X face
        {-1, -1,  1}, {-1, -1, -1}, {-1,  1, -1},
        {-1,  1, -1}, {-1,  1,  1}, {-1, -1,  1},

        // +Y face
        {-1, 1, -1}, {1, 1, -1}, {1, 1, 1},
        {1, 1, 1}, {-1, 1, 1}, {-1, 1, -1},

        // -Y face
        {-1, -1, 1}, {1, -1, 1}, {1, -1, -1},
        {1, -1, -1}, {-1, -1, -1}, {-1, -1, 1},

        // +Z face
        {-1, -1, 1}, {-1, 1, 1}, {1, 1, 1},
        {1, 1, 1}, {1, -1, 1}, {-1, -1, 1},

        // -Z face
        {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, 1, -1}, {-1, -1, -1}, {1, -1, -1}
    };


    VkDeviceSize bufferSize = sizeof(glm::vec3) * cubeVertices.size();

    // Create a staging buffer
    ManagedBuffer staging = m_bufferManager->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    );

    // Copy data to staging buffer
    void* data;
    vmaMapMemory(m_bufferManager->allocator(), staging.allocation, &data);
    memcpy(data, cubeVertices.data(), static_cast<size_t>(bufferSize));
    vmaUnmapMemory(m_bufferManager->allocator(), staging.allocation);

    // Create device-local vertex buffer
    m_cubeVertexBuffer = m_bufferManager->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // Copy from staging to device buffer
    m_bufferManager->copyBuffer(staging.buffer, m_cubeVertexBuffer.buffer, bufferSize);

    // Get device address
    VkBufferDeviceAddressInfo addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = m_cubeVertexBuffer.buffer;
    m_vertexBufferAddress = vkGetBufferDeviceAddress(m_context->device(), &addressInfo);
}

void CubeMapRenderer::createDiffuseIrradiancePipeline()
{
VkDevice device = m_context->device();

    // Descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder(device);
    m_diffuseIrradianceDescriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    // Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };

    m_diffuseIrradianceDescriptorManager = std::make_unique<MainDescriptorManager>(
        device,
        m_diffuseIrradianceDescriptorLayout->handle(),
        poolSizes,
        1
    );

    // Pipeline configuration
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable = VK_FALSE;

    PipelineConfig config{};
    config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/equirect_to_cube_vert.spv";
    config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/diffuse_irradiance_frag.spv";

    config.inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    config.viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    config.rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    config.multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    config.depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE
    };

    config.colorBlendAttachments = {blendAttachment};
    config.colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &blendAttachment
    };

    config.dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    config.rendering = renderingInfo;

    // Push constant range
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(CubeMapPushConstants);

    m_diffuseIrradiancePipeline = std::make_unique<Pipeline>(
        m_context,
        m_diffuseIrradianceDescriptorLayout->handle(),
        config,
        pushConstant
    );
}

CubeMapRenderer::CubeMap CubeMapRenderer::createCubeMap(uint32_t size, VkFormat format) const {
    CubeMap cubeMap;
    cubeMap.texture = m_textureManager->createCubeTexture(
       size, format,
       VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
       VMA_MEMORY_USAGE_GPU_ONLY
   );

    createCubeFaceViews(cubeMap);
    return cubeMap;
}

static int imageViews = 0;
static int cubeMapViews = 0;
void CubeMapRenderer::createCubeFaceViews(CubeMap& cubeMap) const {
    VkDevice device = m_context->device();

    // Create individual face views
    for (uint32_t face = 0; face < 6; ++face) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = cubeMap.texture.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = cubeMap.texture.format;
        viewInfo.subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = face,
            .layerCount = 1
        };

        if (vkCreateImageView(device, &viewInfo, nullptr, &cubeMap.faceViews[face]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cube map face view");
        }

        DeletionQueue::get().pushFunction("CubeFaceView_" + std::to_string(++imageViews),
            [device, view = cubeMap.faceViews[face]]() {
                vkDestroyImageView(device, view, nullptr);
            });
    }

    // Create a cube map view (for sampling)
    VkImageViewCreateInfo cubeViewInfo{};
    cubeViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    cubeViewInfo.image = cubeMap.texture.image;
    cubeViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    cubeViewInfo.format = cubeMap.texture.format;
    cubeViewInfo.subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 6
    };

    if (vkCreateImageView(device, &cubeViewInfo, nullptr, &cubeMap.cubemapView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cube map view");
    }

    DeletionQueue::get().pushFunction("CubeMapView_" + std::to_string(++cubeMapViews), [device, view = cubeMap.cubemapView]() {
        vkDestroyImageView(device, view, nullptr);
    });
}

void CubeMapRenderer::renderEquirectToCube(VkCommandBuffer cmd,
                                         const ManagedTexture& equirectTexture,
                                         const CubeMap& cubeMap) const {

    // Add equirect texture transition
    ImageTransitionManager::transitionImageLayout(
        cmd, equirectTexture.image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        1, 1
    );

    // Update descriptor set with actual texture data
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = m_equirectSampler;
    imageInfo.imageView = equirectTexture.view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


    std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates = {
        {
            .binding = 0,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = &imageInfo,
            .descriptorCount = 1,
            .isImage = true
        }
    };
    m_descriptorManager->updateDescriptorSet(0, updates);

    // Transition cubemap to render target layout
    ImageTransitionManager::transitionImageLayout(
        cmd, cubeMap.texture.image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        6, 1  // 6 faces, 1 mip
    );

    // Precomputed view matrices for each face
    const std::array<glm::mat4, 6> viewMatrices = {
        // +X, -X, +Y, -Y, +Z, -Z
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    proj[1][1] *= -1;  // Flip Y for Vulkan

    for (uint32_t face = 0; face < 6; ++face) {
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = cubeMap.faceViews[face];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = {{0.0f, 0.0f, 0.0f, 1.0f}};

        VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.renderArea = {{0, 0}, {cubeMap.texture.width, cubeMap.texture.height}};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(cmd, &renderInfo);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());

        VkViewport viewport{0.0f, 0.0f,
                           static_cast<float>(cubeMap.texture.width),
                           static_cast<float>(cubeMap.texture.height),
                           0.0f, 1.0f};
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{{0, 0}, {cubeMap.texture.width, cubeMap.texture.height}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               m_pipeline->layout(), 0, 1,
                               &m_descriptorManager->getDescriptorSets()[0],
                               0, nullptr);

        // Push constants with buffer address, viewProj matrix and face index
        CubeMapPushConstants pushConstants{};
        pushConstants.vertexBufferAddress = m_vertexBufferAddress;
        pushConstants.viewProj = proj * viewMatrices[face];
        pushConstants.faceIndex = face;

        vkCmdPushConstants(cmd, m_pipeline->layout(),
                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                          0, sizeof(pushConstants), &pushConstants);

        vkCmdDraw(cmd, 36, 1, 0, 0);

        vkCmdEndRendering(cmd);
    }

    // Final transition to shader read
    ImageTransitionManager::transitionImageLayout(
        cmd, cubeMap.texture.image,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        6, 1
    );
}

CubeMapRenderer::CubeMap CubeMapRenderer::createDiffuseIrradianceMap(VkCommandBuffer cmd, const CubeMap &environmentMap, uint32_t size) {
 CubeMap irradianceMap = createCubeMap(size, VK_FORMAT_R16G16B16A16_SFLOAT);

    if (!m_diffuseIrradiancePipeline) {
        createDiffuseIrradiancePipeline();
    }

    // Update descriptor set with environment map
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = m_equirectSampler; // Or create a new sampler if needed
    imageInfo.imageView = environmentMap.cubemapView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates = {
        {
            .binding = 0,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = &imageInfo,
            .descriptorCount = 1,
            .isImage = true
        }
    };
    m_diffuseIrradianceDescriptorManager->updateDescriptorSet(0, updates);


    // Transition irradiance map to render a target
    ImageTransitionManager::transitionImageLayout(
        cmd, irradianceMap.texture.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        6, 1
    );

    // Same view matrices as in renderEquirectToCube
    const std::array<glm::mat4, 6> viewMatrices = {
        // +X, -X, +Y, -Y, +Z, -Z
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
          glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
          glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
          glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
          glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
          glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    proj[1][1] *= -1;  // Flip Y for Vulkan

    for (uint32_t face = 0; face < 6; ++face) {
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = irradianceMap.faceViews[face];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = {{0.0f, 0.0f, 0.0f, 1.0f}};

        VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.renderArea = {{0, 0}, {size, size}};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(cmd, &renderInfo);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_diffuseIrradiancePipeline->handle());

        VkViewport viewport{0.0f, 0.0f, static_cast<float>(size), static_cast<float>(size), 0.0f, 1.0f};
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{{0, 0}, {size, size}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               m_diffuseIrradiancePipeline->layout(), 0, 1,
                               &m_diffuseIrradianceDescriptorManager->getDescriptorSets()[0],
                               0, nullptr);

        CubeMapPushConstants pushConstants{};
        pushConstants.vertexBufferAddress = m_vertexBufferAddress;
        pushConstants.viewProj = proj * viewMatrices[face];
        pushConstants.faceIndex = face;

        vkCmdPushConstants(cmd, m_diffuseIrradiancePipeline->layout(),
                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                          0, sizeof(pushConstants), &pushConstants);

        vkCmdDraw(cmd, 36, 1, 0, 0);

        vkCmdEndRendering(cmd);
    }

    // Transition to shader read
    ImageTransitionManager::transitionImageLayout(
        cmd, irradianceMap.texture.image,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        6, 1
    );

    return irradianceMap;
}

void CubeMapRenderer::createPipelines() {

    VkDevice device = m_context->device();

    // Create a descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder(device);
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    // Create a descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };

    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        device,
        m_descriptorLayout->handle(),
        poolSizes,
        1
    );

    // Create sampler for equirect textures
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_equirectSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create equirect sampler");
    }

    DeletionQueue::get().pushFunction("EquirectSampler", [device, sampler = m_equirectSampler]() {
        vkDestroySampler(device, sampler, nullptr);
    });


    // Pipeline configuration
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable = VK_FALSE;

    PipelineConfig config{};
    config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/equirect_to_cube_vert.spv";
    config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/equirect_to_cube_frag.spv";

    // Vertex input state (none - using vertex pulling)
    config.inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    config.viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    config.rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    config.multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    config.depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE
    };

    config.colorBlendAttachments = {blendAttachment};
    config.colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &blendAttachment
    };

    config.dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    config.rendering = renderingInfo;

    // Push constant range
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(CubeMapPushConstants);

    m_pipeline = std::make_unique<Pipeline>(
        m_context,
        m_descriptorLayout->handle(),
        config,
        pushConstant
    );
}
