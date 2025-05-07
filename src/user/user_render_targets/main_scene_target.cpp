#include "main_scene_target.h"
#include "data_structures.h"
#include "depth_format.h"
#include "loaders/gltf_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "render_pass_executor.h"
#include "descriptors/descriptor_set_layout_builder.h"
#include "user_executors/main_scene_executor.h"


void MainSceneTarget::initialize(const SharedResources& shared) {
    m_shared = &shared;

    // Resources
    m_texture = m_shared->textureManager->loadTexture(TEXTURE_PATH);
    loadModel(MODEL_PATH);

    createBuffers();
    createDescriptors();
    createRenderingResources();
}

void MainSceneTarget::createPipeline() {
    // Define dynamic states
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    // Define color blend attachment state
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;

    // Create color blend state
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkFormat colorFormat = m_shared->swapChain->format();
    VkPipelineRenderingCreateInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat = m_shared->depthFormat
    };

    PipelineConfig pipelineConfig{
        .vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/shader_vert.spv",
        .fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/shader_frag.spv",

        .inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        },

        .viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1
        },

        .rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f
        },

        .multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE
        },

        .depthStencil = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE
        },

        .colorBlendAttachments = { colorBlendAttachment },
        .colorBlending = colorBlending,

        .dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        },

        .rendering = renderingInfo
    };

    m_pipeline = std::make_unique<Pipeline>(
        m_shared->context,
        m_descriptorLayout->handle(),
        pipelineConfig
    );
}

void MainSceneTarget::createRenderingResources() {
    createPipeline();

    MainSceneExecutor::Resources mainResources{
        .pipeline = m_pipeline->handle(),
        .pipelineLayout = m_pipeline->layout(),
        .vertexBufferAddress = m_deviceAddress,
        .indexBuffer = m_indexBuffer.handle(),
        .descriptorSets = m_descriptorManager->getDescriptorSets(),
        .indices = m_indices,
        .extent = m_shared->swapChain->extent(),
        .colorImageViews = m_shared->swapChain->imagesViews(),
        .depthImageView = m_shared->depthImageView,
        .clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        .clearDepth = {.depthStencil = {1.0f, 0}},
        .swapChain = m_shared->swapChain
        };

    m_executor = std::make_unique<MainSceneExecutor>(mainResources);
}

void MainSceneTarget::createDescriptors() {
    DescriptorSetLayoutBuilder layoutBuilder(m_shared->context->device());
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT /** 68*/}
    };

    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_descriptorLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    // Update descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i].handle();
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_texture.view;
        imageInfo.sampler = m_texture.sampler;

        std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates;
        updates.push_back(MainDescriptorManager::DescriptorUpdateInfo{
            .binding = 0,
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .bufferInfo = bufferInfo,
            .isImage = false
        });
        updates.push_back(MainDescriptorManager::DescriptorUpdateInfo{
            .binding = 1,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = imageInfo,
            .isImage = true
        });

        m_descriptorManager->updateDescriptorSet(i, updates);
    }
}

void MainSceneTarget::render(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    updateUniformBuffers();

    m_executor->begin(commandBuffer, imageIndex);
    m_executor->execute(commandBuffer);
    m_executor->end(commandBuffer);
}


void MainSceneTarget::recreateSwapChain() {
    vkDeviceWaitIdle(m_shared->context->device());

    // Create new executor with updated resources, using the newly updated depth view from shared resources
    MainSceneExecutor::Resources mainResources{
        .pipeline = m_pipeline->handle(),
        .pipelineLayout = m_pipeline->layout(),
        .vertexBufferAddress = m_deviceAddress,
        .indexBuffer = m_indexBuffer.handle(),
        .descriptorSets = m_descriptorManager->getDescriptorSets(),
        .indices = m_indices,
        .extent = m_shared->swapChain->extent(),
        .colorImageViews = m_shared->swapChain->imagesViews(),
        .depthImageView = m_shared->depthImageView,  // Use the updated depth view from shared resources
        .clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        .clearDepth = {.depthStencil = {1.0f, 0}},
        .swapChain = m_shared->swapChain
    };

    // Create new executor
    m_executor = std::make_unique<MainSceneExecutor>(mainResources);
}

void MainSceneTarget::cleanup() {
    vkDeviceWaitIdle(m_shared->context->device());
    m_uniformBuffers.clear();
}

void MainSceneTarget::updateUniformBuffers() const {
    m_uniformBuffers[m_shared->currentFrame].update(m_shared->swapChain->extent(), m_shared->camera);
}


void MainSceneTarget::loadModel(const std::string& modelPath) {
    GLTFModel gltfModel;
    if (!GLTFLoader::LoadFromFile(modelPath, gltfModel)) {
        throw std::runtime_error("Failed to load GLTF model");
    }

    // Load materials and textures
    m_modelTextures.resize(gltfModel.materials.size());
    for (size_t i = 0; i < gltfModel.materials.size(); ++i) {
        if (gltfModel.materials[i].baseColorTexture >= 0) {
            const auto& texInfo = gltfModel.textures[gltfModel.materials[i].baseColorTexture];
            m_modelTextures[i] = m_shared->textureManager->loadTexture(texInfo.uri);
        }
    }
    // Store geometry data
    m_vertices = gltfModel.vertices;
    m_indices = gltfModel.indices;

    // Convert GLTFPrimitive to GLTFPrimitiveData
    m_primitives.clear();
    m_primitives.reserve(gltfModel.primitives.size());
    for (const auto& srcPrim : gltfModel.primitives) {
        m_primitives.push_back({
            .indexOffset = srcPrim.indexOffset,
            .indexCount = srcPrim.indexCount,
            .materialIndex = srcPrim.materialIndex
        });
    }
}

void MainSceneTarget::createBuffers() {
    m_ssboBuffer = SSBOBuffer(m_shared->bufferManager, m_shared->commandManager, m_shared->allocator,
                              m_vertices.data(), sizeof(Vertex) * m_vertices.size());

    m_deviceAddress = m_ssboBuffer.getDeviceAddress(m_shared->context->device());

    m_indexBuffer = IndexBuffer(m_shared->bufferManager, m_shared->commandManager, m_shared->allocator, m_indices);

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    m_uniformBuffers.clear();
    m_uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_uniformBuffers.emplace_back(m_shared->bufferManager, m_shared->allocator, bufferSize);
    }
}
