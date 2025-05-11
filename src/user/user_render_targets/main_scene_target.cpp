#include "main_scene_target.h"
#include "data_structures.h"
#include "depth_format.h"
#include "loaders/gltf_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <iostream>

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
        .swapChain = m_shared->swapChain,
        .primitives = m_primitives,
        .currentFrame = m_shared->currentFrame
        };

    m_executor = std::make_unique<MainSceneExecutor>(mainResources);
}

void MainSceneTarget::createDescriptors() {
    auto textureCount = static_cast<uint32_t>(m_modelTextures.size());

    // Add a check here:
    if (textureCount == 0) {
        throw std::runtime_error("No textures were loaded!");
    }


    DescriptorSetLayoutBuilder layoutBuilder(m_shared->context->device());
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, textureCount)
        .build();

    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textureCount}
    };

    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_descriptorLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    struct FrameData {
        VkDescriptorBufferInfo bufferInfo;
        std::vector<VkDescriptorImageInfo> imageInfos;
    };


    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Initialize buffer info for this frame
        m_frameData[i].bufferInfo = {
            .buffer = m_uniformBuffers[i].handle(),
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        // Initialize image infos for this frame
        m_frameData[i].imageInfos.clear();
        for (const auto& texture : m_modelTextures) {
            m_frameData[i].imageInfos.push_back(VkDescriptorImageInfo{
                .sampler = texture.sampler,
                .imageView = texture.view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }


        // Build descriptor updates
        std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates;
        updates.push_back({
            .binding = 0,
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .bufferInfo = &m_frameData[i].bufferInfo,
            .descriptorCount = 1,
            .isImage = false
        });

        updates.push_back({
            .binding = 1,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfo = m_frameData[i].imageInfos.data(),
            .descriptorCount = static_cast<uint32_t>(m_frameData[i].imageInfos.size()),
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
        .swapChain = m_shared->swapChain,
        .primitives = m_primitives,
        .currentFrame = m_shared->currentFrame

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

    // Clear existing textures and create a texture index mapping
    m_modelTextures.clear();
    std::unordered_map<std::string, size_t> texturePathToIndex;

    // First, load all textures and maintain their original order
    for (const auto& texInfo : gltfModel.textures) {
        std::string texturePath = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + texInfo.uri;
//
        // Check if we already loaded this texture
        auto it = texturePathToIndex.find(texturePath);
        if (it == texturePathToIndex.end()) {
            texturePathToIndex[texturePath] = m_modelTextures.size();
            m_modelTextures.push_back(m_shared->textureManager->loadTexture(texturePath));
        }
    }

    // Store geometry data
    m_vertices = gltfModel.vertices;
    m_indices = gltfModel.indices;

    // Convert primitives, maintaining correct texture indices
    m_primitives.clear();
    m_primitives.reserve(gltfModel.primitives.size());

    for (const auto& srcPrim : gltfModel.primitives) {
        uint32_t textureIndex = 0; // Default texture index

        if (srcPrim.materialIndex >= 0) {
            const auto& material = gltfModel.materials[srcPrim.materialIndex];
            if (material.baseColorTexture >= 0) {
                // Get the texture URI
                const auto& texInfo = gltfModel.textures[material.baseColorTexture];
                std::string texturePath = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + texInfo.uri;
                // Get the correct index from our mapping
                textureIndex = texturePathToIndex[texturePath];
            }
        }

        m_primitives.push_back({
            .indexOffset = srcPrim.indexOffset,
            .indexCount = srcPrim.indexCount,
            .materialIndex = textureIndex
        });
    }

    // Debug print
    std::cout << "Loaded textures count: " << m_modelTextures.size() << std::endl;
    std::cout << "Primitives count: " << m_primitives.size() << std::endl;
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
