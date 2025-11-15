#include "shadow_pass.h"

#include "config.h"
#include "deletion_queue.h"
#include "image_transition_manager.h"
#include "descriptors/descriptor_set_layout_builder.h"

#ifdef USE_TINYGLTF
    #include "loaders/gltf_loader.h"
#endif

void ShadowPass::initialize(const RenderTarget::SharedResources &shared, MainSceneGlobalData &globalData,
                            PassDependencies &dependencies) {

    m_globalData = &globalData;
    m_dependencies = &dependencies;
    m_shared = &shared;

    createLightMatrices();
    createShadowMapTexture();
    createUniformBuffers();
    createDescriptors();
    createPipeline();

}

void ShadowPass::cleanup() {
}

void ShadowPass::recreateSwapChain() {
}

void ShadowPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t) {
    // Transition shadow map to depth attachment layout
    ImageTransitionManager::transitionDepthAttachment(
        cmd, m_dependencies->shadowMap->image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    // Set up depth attachment for dynamic rendering
    VkRenderingAttachmentInfo depthAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_dependencies->shadowMap->view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {.depthStencil = {1.0f, 0}}
    };

    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE}},
        .layerCount = 1,
        .pDepthAttachment = &depthAttachment
    };

    vkCmdBeginRendering(cmd, &renderingInfo);

    // Set viewport/scissor
    VkViewport viewport = {
        0.0f, 0.0f,
        static_cast<float>(SHADOW_MAP_SIZE),
        static_cast<float>(SHADOW_MAP_SIZE),
        0.0f, 1.0f
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {{0, 0}, {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());

    // Bind descriptor set
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipeline->layout(),
        0, 1,
        &m_descriptorManager->getDescriptorSets()[frameIndex],
        0, nullptr
    );

    vkCmdBindIndexBuffer(cmd, m_globalData->indexBuffer.handle(), 0, VK_INDEX_TYPE_UINT32);

    // Draw all meshes
    for (const auto& primitive : m_globalData->primitives) {
        ShadowPushConstants pc = {
            .vertexBufferAddress = m_globalData->vertexBufferAddress,
            .modelScale = globalScale,
            .baseColorTextureIndex = primitive.materialIndex
        };

        vkCmdPushConstants(
            cmd,
            m_pipeline->layout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(ShadowPushConstants),
            &pc
        );

        vkCmdDrawIndexed(
            cmd, primitive.indexCount, 1,
            primitive.indexOffset, 0, 0
        );
    }

    vkCmdEndRendering(cmd);

    // Transition to shader read-only layout for main pass usage
    ImageTransitionManager::transitionDepthAttachment(
        cmd, m_dependencies->shadowMap->image,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    );
}

void ShadowPass::createPipeline() {
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    PipelineConfig config;
    config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/shadows_vert.spv";
    config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/shadows_frag.spv";
    config.inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    config.viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    config.multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    config.rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_TRUE,
        .depthBiasConstantFactor = 1.25f,
        .depthBiasSlopeFactor = 1.75f,
        .lineWidth = 1.0f
    };

    config.depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };

    config.dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // Dynamic rendering configuration
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.depthAttachmentFormat = m_shadowMapTexture.format;
    config.rendering = renderingInfo;

    m_pipeline = std::make_unique<Pipeline>(
        m_shared->context,
        m_descriptorLayout->handle(),
        config
    );
}

void ShadowPass::createDescriptors() {
    DescriptorSetLayoutBuilder layoutBuilder(m_shared->context->device());
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                   static_cast<uint32_t>(m_globalData->modelTextures.size()))
        .build();

    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
 static_cast<uint32_t>(m_globalData->modelTextures.size() * MAX_FRAMES_IN_FLIGHT)}
    };

    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_descriptorLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = m_directionalLightingBuffer.handle(),
            .offset = 0,
            .range = sizeof(DirectionalLightData)
        };

        m_globalData->frameData[i].directionalLightBufferInfo = bufferInfo;
        std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates = {
            {
                .binding = 0,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .bufferInfo = &bufferInfo,
                .descriptorCount = 1,
                .isImage = false
            },
            {
                .binding = 1,
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .imageInfo = m_globalData->frameData[i].textureImageInfos.data(),
                .descriptorCount = static_cast<uint32_t>(m_globalData->modelTextures.size()),
                .isImage = true
            },
        };
        m_descriptorManager->updateDescriptorSet(i, updates);
    }
}

void ShadowPass::createLightMatrices() const {
    const glm::vec3 sceneCenter = (m_globalData->sceneAABB.min + m_globalData->sceneAABB.max) / 2.0f;
    const glm::vec3 lightDirection = directionalLight.directionalLightDirection;

    const std::vector<glm::vec3> corners = {
        {m_globalData->sceneAABB.min.x,  m_globalData->sceneAABB.min.y, m_globalData->sceneAABB.min.z},
        {m_globalData->sceneAABB.max.x,  m_globalData->sceneAABB.min.y, m_globalData->sceneAABB.min.z},
        {m_globalData->sceneAABB.min.x,  m_globalData->sceneAABB.max.y, m_globalData->sceneAABB.min.z},
        {m_globalData->sceneAABB.max.x,  m_globalData->sceneAABB.max.y, m_globalData->sceneAABB.min.z},
        {m_globalData->sceneAABB.min.x,  m_globalData->sceneAABB.min.y, m_globalData->sceneAABB.max.z},
        {m_globalData->sceneAABB.max.x,  m_globalData->sceneAABB.min.y, m_globalData->sceneAABB.max.z},
        {m_globalData->sceneAABB.min.x,  m_globalData->sceneAABB.max.y, m_globalData->sceneAABB.max.z},
        {m_globalData->sceneAABB.max.x,  m_globalData->sceneAABB.max.y, m_globalData->sceneAABB.max.z},
    };

    //Project corners onto light direction
    float minProj = FLT_MAX;
    float maxProj = -FLT_MAX;
    for (const auto& corner : corners) {
        const float proj = glm::dot(corner, lightDirection);
        minProj = std::min(minProj, proj);
        maxProj = std::max(maxProj, proj);
    }

    // Calculate distance and position
    const float distance = maxProj - glm::dot(sceneCenter, lightDirection);
    const glm::vec3 lightPosition = sceneCenter - lightDirection * distance;
    directionalLight.directionalLightPosition = lightPosition;

    // Calculate safe up vector
    const glm::vec3 up = glm::abs(glm:: dot(lightDirection, glm::vec3(0.f, 1.f, 0.f))) > 0.99f
    ? glm::vec3(0.f, 0.f, 1.f)
    : glm::vec3(0.f, 1.f, 0.f);

    // Use lightPosition with center to generate view matrix
    directionalLight.view = glm::lookAt(lightPosition, sceneCenter, up);

    // Find min/max in view/light space
    glm::vec3 minLightSpace(FLT_MAX);
    glm::vec3 maxLightSpace(-FLT_MAX);
    for (const auto& corner : corners) {
        const glm::vec3 transformedCorner = glm::vec3(directionalLight.view * glm::vec4(corner, 1.0f));
        minLightSpace = glm::min(minLightSpace, transformedCorner);
        maxLightSpace = glm::max(maxLightSpace, transformedCorner);
    }

    // Create orthographic projection matrix
    const float nearZ = 0.0f;
    const float farZ = maxLightSpace.z - minLightSpace.z;
    directionalLight.projection = glm::ortho(minLightSpace.x, maxLightSpace.x,
                                                minLightSpace.y, maxLightSpace.y,
                                                nearZ, farZ);
    directionalLight.projection[1][1] *= -1; // Invert y-axis
}

void ShadowPass::createUniformBuffers() {
    m_directionalLightingBuffer = UniformBuffer(
        m_shared->bufferManager,
        m_shared->allocator,
        sizeof(DirectionalLightData)
        );

    m_directionalLightingBuffer.update(directionalLight);
}

void ShadowPass::createShadowMapTexture() {
    VkDevice device = m_shared->context->device();

    m_shadowMapTexture = m_shared->textureManager->createTexture(
        SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        false, "ShadowMapTexture"
    );
    m_shadowMapTexture.format = VK_FORMAT_D32_SFLOAT;

    // Create sampler with compare enabled
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;

    vkCreateSampler(m_shared->context->device(), &samplerInfo, nullptr, &m_shadowMapTexture.sampler);

    VkSampler mapTextureSamplerCopy = m_shadowMapTexture.sampler;

    DeletionQueue::get().pushFunction("ShadowMapSampler_" + std::to_string(TextureManager::getSamplerIndex()), [device, mapTextureSamplerCopy]() {
            vkDestroySampler(device, mapTextureSamplerCopy, nullptr);
        });

    m_dependencies->shadowMap = &m_shadowMapTexture;

}
