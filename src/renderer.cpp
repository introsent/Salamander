#include "renderer.h"
#include "core/image_views.h"
#include "rendering/depth_format.h"
#include "rendering/descriptors/descriptor_set_layout.h"
#include "deletion_queue.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <chrono>
#include <stdexcept>

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "rendering/pipeline.h"

Renderer::Renderer(Context* context,
                   Window* window,
                   VmaAllocator allocator,
                   const std::string& modelPath,
                   const std::string& texturePath)
    : m_context(context)
    , m_window(window)
    , m_allocator(allocator)
{
    initializeVulkan();

    // Command, buffer, texture managers setup remains the same
    m_commandManager = std::make_unique<CommandManager>(
        m_context->device(),
        m_context->findQueueFamilies(m_context->physicalDevice()).graphicsFamily.value(),
        m_context->graphicsQueue()
    );

    m_bufferManager = std::make_unique<BufferManager>(
        m_context->device(), m_allocator, m_commandManager.get()
    );

    m_textureManager = std::make_unique<TextureManager>(
        m_context->device(), m_allocator, m_commandManager.get(), m_bufferManager.get()
    );

    // Depth image setup remains the same
    m_depthImage = m_textureManager->createTexture(
        m_swapChain->extent().width,
        m_swapChain->extent().height,
        m_depthFormat->handle(),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        false
    );

    m_framebufferManager = std::make_unique<FramebufferManager>(
        m_context,
        &m_imageViews->views(),
        m_swapChain->extent(),
        m_depthImage.view
    );

    m_textureImage = m_textureManager->loadTexture(texturePath);

    loadModel(modelPath);

    m_vertexBuffer = VertexBuffer(m_bufferManager.get(), m_commandManager.get(), m_allocator, m_vertices);
    m_indexBuffer = IndexBuffer(m_bufferManager.get(), m_commandManager.get(), m_allocator, m_indices);
    createUniformBuffers();

    // Update descriptor sets using the new MainDescriptorManager
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i].handle();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImage.view;
        imageInfo.sampler = m_textureImage.sampler;

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

        m_mainDescriptorManager->updateDescriptorSet(i, updates);
    }

    initializeGraphicsPipeline();
    initializeRenderingResources();


    createCommandBuffers();
    createSyncObjects();
}


void Renderer::initializeVulkan() {
    // Basic Vulkan resources
    m_swapChain = std::make_unique<SwapChain>(m_context, m_window);
    m_imageViews = std::make_unique<ImageViews>(m_context, m_swapChain.get());
    m_depthFormat = std::make_unique<DepthFormat>(m_context->physicalDevice());

    // Create descriptor set layouts
    auto layoutBuilder = DescriptorSetLayoutBuilder(m_context->device());
    m_mainSceneLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    // Create descriptor managers
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT }
    };

    m_mainDescriptorManager = std::make_unique<MainDescriptorManager>(
        m_context->device(),
        m_mainSceneLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    m_imguiDescriptorManager = std::make_unique<ImGuiDescriptorManager>(m_context->device());
}

void Renderer::initializeGraphicsPipeline() {
    // Create render passes first as they're needed for pipeline creation
    RenderPass::Config mainPassConfig{
        .colorFormat = m_swapChain->format(),
        .depthFormat = m_depthFormat->handle(),
        .colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .colorInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .colorFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .depthInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .depthFinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    m_mainRenderPass = RenderPass::create(m_context, mainPassConfig);

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
        m_context,
        m_mainRenderPass->handle(),
        m_mainSceneLayout->handle(),
        pipelineConfig
    );
}

void Renderer::initializeRenderingResources() {
    // Create ImGui render pass
    RenderPass::Config imguiPassConfig{
        .colorFormat = m_swapChain->format(),
        .depthFormat = m_depthFormat->handle(),
        .colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .colorInitialLayout =  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .colorFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .depthInitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .depthFinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    m_imguiRenderPass = RenderPass::create(m_context, imguiPassConfig);

    // Create and set up pass executors
    m_mainSceneFramebuffers = m_framebufferManager->createFramebuffersForRenderPass(m_mainRenderPass->handle());
    m_imguiFramebuffers = m_framebufferManager->createFramebuffersForRenderPass(m_imguiRenderPass->handle());

    // Main scene pass executor
    MainScenePassExecutor::Resources mainResources{
        .pipeline = m_pipeline->handle(),
        .pipelineLayout = m_pipeline->layout(),
        .vertexBuffer = m_vertexBuffer.handle(),
        .indexBuffer = m_indexBuffer.handle(),
        .descriptorSets = m_mainDescriptorManager->getDescriptorSets(),
        .indices = m_indices,
        .framebuffers = m_mainSceneFramebuffers.framebuffers,
        .extent = m_swapChain->extent()
    };
    m_passExecutors.push_back(
        std::make_unique<MainScenePassExecutor>(m_mainRenderPass.get(), mainResources)
    );

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForVulkan(m_window->handle(), true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_context->instance();
    init_info.PhysicalDevice = m_context->physicalDevice();
    init_info.Device = m_context->device();
    init_info.QueueFamily = m_context->findQueueFamilies(m_context->physicalDevice()).graphicsFamily.value();
    init_info.Queue = m_context->graphicsQueue();
    init_info.DescriptorPool = m_imguiDescriptorManager->getPool();
    init_info.MinImageCount = m_swapChain->images().size();
    init_info.ImageCount = m_swapChain->images().size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.RenderPass = m_imguiRenderPass->handle();

    ImGui_ImplVulkan_Init(&init_info);

    // Upload ImGui fonts
    auto commandBuffer = m_commandManager->beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
    m_commandManager->endSingleTimeCommands(commandBuffer);
    ImGui_ImplVulkan_DestroyFontsTexture();

    // ImGui pass executor
    ImGuiPassExecutor::Resources imguiResources{
        .framebuffers = m_imguiFramebuffers.framebuffers,
        .extent = m_swapChain->extent(),
        .imguiContext = ImGui::GetCurrentContext()
    };
    m_passExecutors.push_back(
        std::make_unique<ImGuiPassExecutor>(m_imguiRenderPass.get(), imguiResources)
    );
}

void Renderer::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_frames.size(); ++i) {
        auto& frame = m_frames[i];

        if (vkCreateSemaphore(m_context->device(), &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(m_context->device(), &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(m_context->device(), &fenceInfo, nullptr, &frame.inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create sync objects for a frame!");
        }

        auto device = m_context->device();

        DeletionQueue::get().pushFunction("ImageSemaphore_" + std::to_string(i),
            [device, sem = frame.imageAvailableSemaphore]() {
                vkDestroySemaphore(device, sem, nullptr);
            });

        DeletionQueue::get().pushFunction("RenderSemaphore_" + std::to_string(i),
            [device, sem = frame.renderFinishedSemaphore]() {
                vkDestroySemaphore(device, sem, nullptr);
            });

        DeletionQueue::get().pushFunction("Fence_" + std::to_string(i),
            [device, f = frame.inFlightFence]() {
                vkDestroyFence(device, f, nullptr);
            });
    }
}


void Renderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    m_uniformBuffers.clear();
    m_uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_uniformBuffers.emplace_back(m_bufferManager.get(), m_allocator, bufferSize);
    }
}

void Renderer::createCommandBuffers() {
    m_frames.resize(MAX_FRAMES_IN_FLIGHT);
    for (auto& frame : m_frames) {
        frame.commandBuffer = m_commandManager->createCommandBuffer();
    }
}


void Renderer::loadModel(const std::string& modelPath) {
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


void Renderer::drawFrame() {
    Frame& currentFrame = m_frames[m_currentFrame];
    vkWaitForFences(m_context->device(), 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        m_context->device(),
        m_swapChain->handle(),
        UINT64_MAX,
        currentFrame.imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &imageIndex
    );

    // Update uniform buffer for current frame
    m_uniformBuffers[m_currentFrame].update(m_swapChain->extent());

    // Reset command buffer and begin recording
    vkResetFences(m_context->device(), 1, &currentFrame.inFlightFence);
    currentFrame.commandBuffer->reset();
    currentFrame.commandBuffer->begin();

    // Begin ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();

    // Execute all render passes
    for (auto& executor : m_passExecutors) {
        executor->begin(currentFrame.commandBuffer->handle(), imageIndex);
        executor->execute(currentFrame.commandBuffer->handle());
        executor->end(currentFrame.commandBuffer->handle());
    }

    currentFrame.commandBuffer->end();

    // Submit command buffer
    VkCommandBuffer commandBufferHandle = currentFrame.commandBuffer->handle();
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { currentFrame.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBufferHandle;

    VkSemaphore signalSemaphores[] = { currentFrame.renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo, currentFrame.inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Present the frame
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_swapChain->handle() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(m_context->presentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::recreateSwapChain() {
    vkDeviceWaitIdle(m_context->device());
    m_swapChain->recreate();
    m_imageViews->recreate(m_swapChain.get());

    // Recreate depth image and update framebuffer manager.
    m_depthImage = m_textureManager->createTexture(
        m_swapChain->extent().width,
        m_swapChain->extent().height,
        m_depthFormat->handle(),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        false
    );
    m_framebufferManager->recreate(&m_imageViews->views(),
        m_swapChain->extent(),
        m_depthImage.view);

    m_mainSceneFramebuffers = m_framebufferManager->createFramebuffersForRenderPass(m_mainRenderPass->handle());
    m_imguiFramebuffers = m_framebufferManager->createFramebuffersForRenderPass(m_imguiRenderPass->handle());

    // Clear existing executors
    m_passExecutors.clear();

    // Recreate main scene pass executor with new extent
    MainScenePassExecutor::Resources mainResources{
        .pipeline = m_pipeline->handle(),
        .pipelineLayout = m_pipeline->layout(),
        .vertexBuffer = m_vertexBuffer.handle(),
        .indexBuffer = m_indexBuffer.handle(),
        .descriptorSets = m_mainDescriptorManager->getDescriptorSets(),
        .indices = m_indices,
        .framebuffers = m_mainSceneFramebuffers.framebuffers,
        .extent = m_swapChain->extent()  // Updated extent
    };
    m_passExecutors.push_back(
        std::make_unique<MainScenePassExecutor>(m_mainRenderPass.get(), mainResources)
    );

    // Recreate ImGui pass executor with new extent
    ImGuiPassExecutor::Resources imguiResources{
        .framebuffers = m_imguiFramebuffers.framebuffers,
        .extent = m_swapChain->extent(),  // Updated extent
        .imguiContext = ImGui::GetCurrentContext()
    };
    m_passExecutors.push_back(
        std::make_unique<ImGuiPassExecutor>(m_imguiRenderPass.get(), imguiResources)
    );

    ImGui_ImplVulkan_SetMinImageCount(m_swapChain->images().size());


}

void Renderer::markFramebufferResized() {
    m_framebufferResized = true;
}


void Renderer::cleanup() {
    vkDeviceWaitIdle(m_context->device());

    m_uniformBuffers.clear();
}

Renderer::~Renderer() {
    cleanup();
}