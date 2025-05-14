#include "renderer.h"
#include "image_views.h"
#include "depth_format.h"
#include "descriptors/descriptor_set_layout.h"
#include "deletion_queue.h"

#include <chrono>
#include <stdexcept>

#include "user_render_targets/main_scene_target.h"
#include "user_render_targets/imgui_target.h"


Renderer::Renderer(Context* context, Window* window, VmaAllocator allocator, Camera* camera)
    : m_context(context), m_window(window), m_allocator(allocator) {

    initializeSharedResources(camera);

    // Create targets
    m_renderTargets.push_back(std::make_unique<MainSceneTarget>());
    m_renderTargets.push_back(std::make_unique<ImGuiTarget>());

    // Initialize targets
    for (auto& target : m_renderTargets) {
        target->initialize(m_sharedResources);
    }

    createCommandBuffers();
    createSyncObjects();
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

void Renderer::createCommandBuffers() {
    m_frames.resize(MAX_FRAMES_IN_FLIGHT);
    for (auto& frame : m_frames) {
        frame.commandBuffer = m_commandManager->createCommandBuffer();
    }
}

void Renderer::initializeSharedResources(Camera* camera) {
    m_swapChain = std::make_unique<SwapChain>(m_context, m_window);
    m_depthFormat = std::make_unique<DepthFormat>(m_context->physicalDevice());

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

    VkExtent2D extent = m_swapChain->extent();
    m_depthImage = m_textureManager->createTexture(
       extent.width,
       extent.height,
       m_depthFormat->handle(),
       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |  VK_IMAGE_USAGE_SAMPLED_BIT,
       VMA_MEMORY_USAGE_GPU_ONLY,
       VK_IMAGE_ASPECT_DEPTH_BIT,
       false
   );


    //SharedResources:
    //    Context* context;
    //    Window* window;
    //    SwapChain* swapChain;
    //    CommandManager* commandManager;
    //    BufferManager* bufferManager;
    //    TextureManager* textureManager;
    //    uint32_t currentFrame;
    //    VmaAllocator allocator;
    //    VkImageView depthImageView;
    //};
    m_sharedResources = {
        .context = m_context,
        .window = m_window,
        .swapChain = m_swapChain.get(),
        .commandManager = m_commandManager.get(),
        .bufferManager = m_bufferManager.get(),
        .textureManager = m_textureManager.get(),
        .currentFrame = m_currentFrame,
        .allocator = m_allocator,
        .depthImageView = m_depthImage.view,
        .depthImage = m_depthImage.image,
        .depthFormat = m_depthFormat->handle(),
        .camera = camera
    };
}


void Renderer::drawFrame() {
    Frame& currentFrame = m_frames[m_currentFrame];

    // Wait for fence before doing anything
    vkWaitForFences(m_context->device(), 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);

    // Try to acquire next image
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
        return;
    }

    m_sharedResources.currentFrame = m_currentFrame;
    for (auto& target : m_renderTargets) {
        target->updateUniformBuffers(); // Move update here
    }

    vkResetFences(m_context->device(), 1, &currentFrame.inFlightFence);

    // Reset and record command buffer
    currentFrame.commandBuffer->reset();
    currentFrame.commandBuffer->begin();

    m_sharedResources.currentFrame = m_currentFrame;
    for (auto& target : m_renderTargets) {
        target->render(currentFrame.commandBuffer->handle(), imageIndex);
    }

    currentFrame.commandBuffer->end();

    // Set up Vulkan Synchronization 2 structures
    VkSemaphoreSubmitInfo waitSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = currentFrame.imageAvailableSemaphore,
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphoreSubmitInfo signalSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = currentFrame.renderFinishedSemaphore,
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkCommandBufferSubmitInfo cmdBufferInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = currentFrame.commandBuffer->handle()
    };

    VkSubmitInfo2 submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdBufferInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalSemaphoreInfo
    };

    if (vkQueueSubmit2(m_context->graphicsQueue(), 1, &submitInfo, currentFrame.inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChain = m_swapChain->handle();
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &currentFrame.renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapChain,
        .pImageIndices = &imageIndex
    };

    result = vkQueuePresentKHR(m_context->presentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        recreateSwapChain();
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


void Renderer::recreateSwapChain() {
    // Wait for all operations to complete
    vkDeviceWaitIdle(m_context->device());

    // Recreate swapchain
    m_swapChain->recreate();

    // Recreate depth image
    m_depthImage = m_textureManager->createTexture(
        m_swapChain->extent().width,
        m_swapChain->extent().height,
        m_depthFormat->handle(),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        false
    );

    // Update depth image
    m_sharedResources.depthImageView = m_depthImage.view;

    // Recreate targets
    for (auto& target : m_renderTargets) {
        target->recreateSwapChain();
    }
}

void Renderer::markFramebufferResized() {
    m_framebufferResized = true;
}


void Renderer::cleanup() {
    for (auto& target : m_renderTargets) {
        target->cleanup();
    }
}

Renderer::~Renderer() {
    cleanup();
}