#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vk_mem_alloc.h" // Include the VMA header


#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>

#include "core/data_structures.h"
#include "core/window.h"
#include "core/context.h"
#include "core/framebuffer_manager.h"
#include "core/image_views.h"
#include "core/swap_chain.h"
#include "rendering/depth_format.h"

#include "rendering/descriptor_manager.h"
#include "rendering/descriptor_set_layout.h"
#include "rendering/pipeline.h"
#include "rendering/render_pass.h"

#include "resources/command_manager.h"
#include "resources/buffer_manager.h"
#include "resources/texture_manager.h"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "./models/viking_room.obj";
const std::string TEXTURE_PATH = "./textures/viking_room.png";

constexpr int MAX_FRAMES_IN_FLIGHT = 2;


const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class HelloTriangleApplication {
public:
    void run() {
        m_window = new Window(WIDTH, HEIGHT, "Salamander");
        m_context = new Context(m_window, enableValidationLayers);
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    VkDebugUtilsMessengerEXT debugMessenger;

    Window* m_window;
    Context* m_context;
    SwapChain* m_swapChain;
    ImageViews* m_imageViews;
    FramebufferManager* m_framebufferManager;

    RenderPass* m_renderPass;
    DescriptorSetLayout* m_descriptorSetLayout;

    DepthFormat* m_depthFormat;

    Pipeline* m_pipeline;

    VkCommandPool commandPool;
    std::vector<std::unique_ptr<CommandBuffer>> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    VmaAllocator allocator;

    CommandManager* m_commandManager = nullptr;
    BufferManager* m_bufferManager = nullptr;
    TextureManager* m_textureManager = nullptr;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    ManagedBuffer m_vertexBuffer;
    ManagedBuffer m_indexBuffer;
    std::vector<ManagedBuffer> m_uniformBuffers;
    std::vector<void*> uniformBuffersMapped;

    DescriptorManager* m_descriptorManager = nullptr;

    ManagedTexture m_textureImage;
    ManagedTexture m_depthImage;

    void cleanup() {
        vkDeviceWaitIdle(m_context->device());

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (uniformBuffersMapped[i]) {
                vmaUnmapMemory(allocator, m_uniformBuffers[i].allocation);
                uniformBuffersMapped[i] = nullptr;
            }
        }

        delete m_textureManager;
        delete m_bufferManager;

        delete m_framebufferManager;
        delete m_imageViews;
        delete m_swapChain;

        delete m_pipeline;

        delete m_renderPass;

        delete m_descriptorManager;

        delete m_descriptorSetLayout;

        // Destroy synchronization objects.
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_context->device(), renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_context->device(), imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_context->device(), inFlightFences[i], nullptr);
        }

        delete m_commandManager;

        vmaDestroyAllocator(allocator);

        delete m_context;
        delete m_window;

        delete m_depthFormat;
    }

    void initVulkan() {
        createAllocator();
        m_swapChain = new SwapChain(m_context, m_window);
        m_imageViews = new ImageViews(m_context, m_swapChain);
        m_depthFormat = new DepthFormat(m_context->physicalDevice());

        m_renderPass = new RenderPass(m_context, m_swapChain->format(), m_depthFormat->handle());
        m_descriptorSetLayout = new DescriptorSetLayout(m_context);
        PipelineConfig configDefault = PipelineFactory::createDefaultPipelineConfig();
        m_pipeline = new Pipeline(m_context, m_renderPass->handle(), m_descriptorSetLayout->handle(), configDefault);
        m_commandManager = new CommandManager(
            m_context->device(),
            m_context->findQueueFamilies(m_context->physicalDevice()).graphicsFamily.value(),
            m_context->graphicsQueue()
        );
        m_bufferManager = new BufferManager(m_context->device(), allocator, m_commandManager);
        m_textureManager = new TextureManager(m_context->device(), allocator, m_commandManager, m_bufferManager);

        m_depthImage = m_textureManager->createTexture(
            m_swapChain->extent().width,
            m_swapChain->extent().height,
            m_depthFormat->handle(),
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            false // no sampler needed for depth
        );
        m_framebufferManager = new FramebufferManager(m_context, &m_imageViews->views(), m_swapChain->extent(), m_renderPass->handle(), m_depthImage.view);
        m_textureImage = m_textureManager->loadTexture(TEXTURE_PATH);

        loadModel();

        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();

        std::vector<VkDescriptorPoolSize> poolSizes(2);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;
        m_descriptorManager = new DescriptorManager(
            m_context->device(),
            m_descriptorSetLayout->handle(),
            poolSizes,
            MAX_FRAMES_IN_FLIGHT
        );
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_uniformBuffers[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_textureImage.view;
            imageInfo.sampler = m_textureImage.sampler;

            m_descriptorManager->updateDescriptorSet(i, bufferInfo, imageInfo);
        }
        createCommandBuffer();
        createSyncObjects();
    }

    void loadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
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
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            // Create and track the uniform buffer
            m_uniformBuffers[i] = m_bufferManager->createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU
            );

            // Persistently map it
            void* mappedData = nullptr;
            vmaMapMemory(allocator, m_uniformBuffers[i].allocation, &mappedData);
            uniformBuffersMapped[i] = mappedData;
        }
    }


    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        ManagedBuffer staging = m_bufferManager->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        void* data;
        vmaMapMemory(allocator, staging.allocation, &data);
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
        vmaUnmapMemory(allocator, staging.allocation);

        m_indexBuffer = m_bufferManager->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );


        VkCommandBuffer commandBuffer = m_commandManager->beginSingleTimeCommands();
        m_bufferManager->copyBuffer(staging.buffer, m_indexBuffer.buffer, bufferSize);
        m_commandManager->endSingleTimeCommands(commandBuffer);
    }

    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        ManagedBuffer staging = m_bufferManager->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        void* data;
        vmaMapMemory(allocator, staging.allocation, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vmaUnmapMemory(allocator, staging.allocation);

        m_vertexBuffer = m_bufferManager->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );

        VkCommandBuffer commandBuffer = m_commandManager->beginSingleTimeCommands();
        m_bufferManager->copyBuffer(staging.buffer, m_vertexBuffer.buffer, bufferSize);
        m_commandManager->endSingleTimeCommands(commandBuffer);
    }

    void createAllocator() {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = m_context->physicalDevice();
        allocatorInfo.device = m_context->device();
        allocatorInfo.instance = m_context->instance();

        if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
            throw std::runtime_error("failed to create VMA allocator!");
        }
    }


    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_context->device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_context->device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_context->device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void createCommandBuffer() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto commandBuffer = m_commandManager->createCommandBuffer();
            commandBuffers.push_back(std::move(commandBuffer));
        }
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(m_context->device(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }


    void recreateSwapChain() {
        VkExtent2D validExtent = m_window->getValidExtent();

        m_swapChain->recreate();
        m_imageViews->recreate(m_swapChain);

        m_depthImage = m_textureManager->createTexture(
            m_swapChain->extent().width,
            m_swapChain->extent().height,
            m_depthFormat->handle(),
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            false // no sampler needed for depth
        );

        m_framebufferManager->recreate(&m_imageViews->views(), m_swapChain->extent(), m_renderPass->handle(), m_depthImage.view);
    }

    void mainLoop() {
        while (!m_window->shouldClose()) {
            m_window->pollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(m_context->device());
    }

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f),
            (float)m_swapChain->extent().width / (float)m_swapChain->extent().height,
            0.1f, 10.0f);
        ubo.proj[1][1] *= -1; // Flip Y for Vulkan

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void drawFrame() {
        vkWaitForFences(m_context->device(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_context->device(), m_swapChain->handle(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer(currentFrame);

        vkResetFences(m_context->device(), 1, &inFlightFences[currentFrame]);

        // Reset and record the command buffer
        commandBuffers[currentFrame]->reset();
        CommandManager::recordCommandBuffer(
			*commandBuffers[currentFrame],
			m_renderPass->handle(),
			m_framebufferManager->framebuffers()[imageIndex],
			m_swapChain->extent(),
			m_pipeline->handle(),
			m_pipeline->layout(),
			m_vertexBuffer.buffer,
			m_indexBuffer.buffer,
			{ m_descriptorManager->getDescriptorSets() },
			currentFrame,
			indices
		);

        const VkCommandBuffer rawCommandBuffer = commandBuffers[currentFrame]->handle();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &rawCommandBuffer;

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
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

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }


    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}