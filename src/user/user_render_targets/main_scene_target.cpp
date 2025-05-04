#include "main_scene_target.h"
#include "../core/data_structures.h"
#include "../rendering/depth_format.h"
#include "../tiny_obj_loader.h"
#include "../../executors/render_pass_executor.h"
#include "../rendering/descriptors/descriptor_set_layout_builder.h"
#include "../executors/main_scene_pass_executor.h"


void MainSceneTarget::initialize(const SharedResources& shared) {
    m_shared = shared;

    createDescriptors();
    // Resources
    m_texture = m_shared.textureManager->loadTexture(TEXTURE_PATH);
    loadModel(MODEL_PATH);
    createBuffers();

    createRenderingResources();
}

void MainSceneTarget::createPipeline() {
    // Define dynamic states
    static const std::array<VkDynamicState, 2> dynamicStates = {
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

    PipelineConfig pipelineConfig{
        .vertShaderPath = "./shaders/shader_vert.spv",
        .fragShaderPath = "./shaders/shader_frag.spv",
        .bindingDescription = Vertex::getBindingDescription(),
        .attributeDescriptions = Vertex::getAttributeDescriptions(),

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
        }
    };

    m_pipeline = std::make_unique<Pipeline>(
        m_shared.context,
        m_renderPass->handle(),
        m_descriptorLayout->handle(),
        pipelineConfig
    );
}

void MainSceneTarget::createRenderingResources() {
    // Render Pass
    RenderPass::Config config{
        .colorFormat = m_shared.swapChain->format(),
        .depthFormat = DepthFormat(m_shared.context->physicalDevice()).handle(),
        .colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .colorInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .colorFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .depthInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .depthFinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    m_renderPass = RenderPass::create(m_shared.context, config);

    createPipeline();

    m_framebuffers = m_shared.framebufferManager->createFramebuffersForRenderPass(m_renderPass->handle());

    MainScenePassExecutor::Resources mainResources{
        .pipeline = m_pipeline->handle(),
        .pipelineLayout = m_pipeline->layout(),
        .vertexBuffer = m_vertexBuffer.handle(),
        .indexBuffer = m_indexBuffer.handle(),
        .descriptorSets = m_descriptorManager->getDescriptorSets(),
        .indices = m_indices,
        .framebuffers = m_framebuffers.framebuffers,
        .extent = m_shared.swapChain->extent()
    };
    m_executor = std::make_unique<MainScenePassExecutor>(m_renderPass.get(), mainResources);
}

void MainSceneTarget::createDescriptors() {
    DescriptorSetLayoutBuilder layoutBuilder(m_shared.context->device());
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT}
    };

    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared.context->device(),
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
    m_executor->begin(commandBuffer, imageIndex);
    m_executor->execute(commandBuffer);
    m_executor->end(commandBuffer);
}

void MainSceneTarget::recreateSwapChain() {
    m_framebuffers = m_shared.framebufferManager->createFramebuffersForRenderPass(m_renderPass->handle());

    MainScenePassExecutor::Resources mainResources{
        .pipeline = m_pipeline->handle(),
        .pipelineLayout = m_pipeline->layout(),
        .vertexBuffer = m_vertexBuffer.handle(),
        .indexBuffer = m_indexBuffer.handle(),
        .descriptorSets = m_descriptorManager->getDescriptorSets(),
        .indices = m_indices,
        .framebuffers = m_framebuffers.framebuffers,
        .extent = m_shared.swapChain->extent()
    };
    m_executor = std::make_unique<MainScenePassExecutor>(m_renderPass.get(), mainResources);
}

void MainSceneTarget::cleanup() {
    vkDeviceWaitIdle(m_shared.context->device());
    m_uniformBuffers.clear();
}

void MainSceneTarget::loadModel(const std::string& modelPath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    if (!LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str())) {
        throw std::runtime_error(warn + err);
    }
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            vertex.color = { 1.0f, 1.0f, 1.0f };
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
            }
            m_indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void MainSceneTarget::createBuffers() {
    m_vertexBuffer = VertexBuffer(m_shared.bufferManager, m_shared.commandManager, m_shared.allocator, m_vertices);
    m_indexBuffer = IndexBuffer(m_shared.bufferManager, m_shared.commandManager, m_shared.allocator, m_indices);

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    m_uniformBuffers.clear();
    m_uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_uniformBuffers.emplace_back(m_shared.bufferManager, m_shared.allocator, bufferSize);
    }
}
