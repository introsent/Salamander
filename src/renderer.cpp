#include "renderer.h"
#include "core/image_views.h"
#include "rendering/depth_format.h"
#include "rendering/descriptor_set_layout.h"
#include "deletion_queue.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <chrono>
#include <stdexcept>

Renderer::Renderer(Context* context,
    Window* window,
    VmaAllocator allocator,
    const std::string& modelPath,
    const std::string& texturePath)
    : m_context(context)
    , m_window(window)
    , m_allocator(allocator)
{
    initVulkan();

    // Command, buffer, texture managers
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

    // Depth image (queues its own cleanup inside TextureManager)
    m_depthImage = m_textureManager->createTexture(
        m_swapChain->extent().width,
        m_swapChain->extent().height,
        m_depthFormat->handle(),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        false
    );

    // Framebuffers (queues cleanup in FramebufferManager)
    m_framebufferManager = std::make_unique<FramebufferManager>(
        m_context,
        &m_imageViews->views(),
        m_swapChain->extent(),
        m_renderPass->handle(),
        m_depthImage.view
    );

    // Texture image (queues cleanup in TextureManager)
    m_textureImage = m_textureManager->loadTexture(texturePath);

    loadModel(modelPath);

    // Vertex/index buffers (managers queue destruction)
    m_vertexBuffer = VertexBuffer(m_bufferManager.get(), m_commandManager.get(), m_allocator, m_vertices);
    m_indexBuffer = IndexBuffer(m_bufferManager.get(), m_commandManager.get(), m_allocator, m_indices);
    createUniformBuffers();  // UniformBuffer destructor unmaps automatically

    // Descriptor manager + update sets
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       MAX_FRAMES_IN_FLIGHT },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT }
    };

    m_descriptorManager = std::make_unique<DescriptorManager>(
        m_context->device(),
        m_descriptorSetLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i].handle();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImage.view;
        imageInfo.sampler = m_textureImage.sampler;

        m_descriptorManager->updateDescriptorSet(
            static_cast<uint32_t>(i), bufferInfo, imageInfo
        );
    }

    createCommandBuffers();
    createSyncObjects();
}


void Renderer::initVulkan() {
    m_swapChain = std::make_unique<SwapChain>(m_context, m_window);
    m_imageViews = std::make_unique<ImageViews>(m_context, m_swapChain.get());
    m_depthFormat = std::make_unique<DepthFormat>(m_context->physicalDevice());
    m_renderPass = std::make_unique<RenderPass>(m_context, m_swapChain->format(), m_depthFormat->handle());
    m_descriptorSetLayout = std::make_unique<DescriptorSetLayout>(m_context);
    PipelineConfig configDefault = PipelineFactory::createDefaultPipelineConfig();
    m_pipeline = std::make_unique<Pipeline>(m_context,
        m_renderPass->handle(),
        m_descriptorSetLayout->handle(),
        configDefault
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
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }

    // Update the uniform buffer for the current frame.
    m_uniformBuffers[m_currentFrame].update(m_swapChain->extent());

    vkResetFences(m_context->device(), 1, &currentFrame.inFlightFence);
    currentFrame.commandBuffer->reset();

    CommandManager::recordCommandBuffer(
        *currentFrame.commandBuffer,
        m_renderPass->handle(),
        m_framebufferManager->framebuffers()[imageIndex],
        m_swapChain->extent(),
        m_pipeline->handle(),
        m_pipeline->layout(),
        m_vertexBuffer.handle(),
        m_indexBuffer.handle(),
        { m_descriptorManager->getDescriptorSets() },
        m_currentFrame,
        m_indices
    );

    VkCommandBuffer rawCommandBuffer = currentFrame.commandBuffer->handle();
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { currentFrame.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &rawCommandBuffer;

    VkSemaphore signalSemaphores[] = { currentFrame.renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo, currentFrame.inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

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
        m_renderPass->handle(),
        m_depthImage.view);
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
