#include "render.h"
#include "depth_format.h"
#include "descriptors/descriptor_set_layout.h"
#include "deletion_queue.h"

#include <chrono>
#include <fstream>
#include <stdexcept>
#include <memory>

#include "debug/render_graph_debug_panel.h"
#include "frame/frame_data.h"
#include "renderer/targets/main_scene_target.h"
#include "renderer/targets/imgui_target.h"
#include "textures/texture_manager.h"

namespace Salamander {
    Render::Render(Core::Context *context, Core::Window *window, VmaAllocator allocator, Scene::Camera *camera)
        : m_context(context), m_window(window), m_allocator(allocator), m_camera(camera) {
        initializeResources(camera);
        createCommandBuffers();
        createSyncObjects();

        auto mainSceneTarget = std::make_unique<Renderer::Targets::MainSceneTarget>(m_dependencies);
        m_mainSceneTarget = mainSceneTarget.get();
        m_renderTargets.push_back(std::move(mainSceneTarget));

        auto imguiTarget = std::make_unique<Renderer::Targets::ImGuiTarget>();
        m_imguiTarget = imguiTarget.get();
        m_renderTargets.push_back(std::move(imguiTarget));

        for (auto &target: m_renderTargets) {
            target->initialize(*m_renderContext);
        }

        m_renderGraphDebugPanel = std::make_unique<Renderer::Debug::RenderGraphDebugPanel>(m_context->device());

        m_imguiTarget->setExtraUiCallback([this](const uint32_t frameIndex) {
            m_renderGraphDebugPanel->draw(m_mainSceneTarget->getRenderGraph(), frameIndex);
        });
    }

    void Render::createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < m_frames.size(); ++i) {
            auto &frame = m_frames[i];

            if (vkCreateSemaphore(m_context->device(), &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore) !=
                VK_SUCCESS ||
                vkCreateSemaphore(m_context->device(), &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore) !=
                VK_SUCCESS ||
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

    void Render::createCommandBuffers() {
        m_frames.resize(Renderer::Frame::MAX_FRAMES_IN_FLIGHT);
        VkExtent2D extent = m_swapChain->extent();

        for (auto &frame: m_frames) {
            frame.commandBuffer = m_commandManager->createCommandBuffer();
            frame.depthTexture = &m_textureManager->createTexture(
                extent.width,
                extent.height,
                m_depthFormat->handle(),
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY,
                VK_IMAGE_ASPECT_DEPTH_BIT,
                false, true, "Depth_Frame_" + std::to_string(&frame - &m_frames[0]) // Unique name per frame
            );
        }
    }

    void Render::initializeResources(Scene::Camera *camera) {
        m_swapChain = std::make_unique<Core::SwapChain>(m_context, m_window);
        m_depthFormat = std::make_unique<Graphics::DepthFormat>(m_context->physicalDevice());

        m_commandManager = std::make_unique<Resources::Buffers::CommandManager>(
            m_context->device(),
            m_context->findQueueFamilies(m_context->physicalDevice()).graphicsFamily.value(),
            m_context->graphicsQueue()
        );

        m_bufferManager = std::make_unique<Resources::Buffers::BufferManager>(
            m_context->device(), m_allocator, m_commandManager.get()
        );

        m_textureManager = std::make_unique<Resources::Textures::TextureManager>(
            m_context->device(), m_context->physicalDevice(), m_allocator, m_commandManager.get(),
            m_bufferManager.get(), m_context->debugMessenger()
        );

        // Create RenderContext
        m_renderContext = std::make_unique<Renderer::Frame::RenderContext>(
            *m_context,
            *m_window,
            *m_swapChain,
            *m_commandManager,
            *m_bufferManager,
            *m_textureManager,
            m_allocator,
            VK_NULL_HANDLE,  // depthImageView will be set per-frame
            m_depthFormat->handle(),
            *camera,
            m_frames
        );
    }


    void Render::drawFrame(float deltaTime) {
        Renderer::Frame::Frame &currentFrame = m_frames[m_currentFrame];

        // wait for fence before doing anything
        vkWaitForFences(m_context->device(), 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_context->device(), 1, &currentFrame.inFlightFence);

        // try to acquire next image
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

        // reset and record command buffer
        currentFrame.commandBuffer->reset();
        currentFrame.commandBuffer->begin();

        for (auto &target: m_renderTargets) {
            target->render(deltaTime, currentFrame.commandBuffer->handle(), imageIndex, m_currentFrame);
        }

        currentFrame.commandBuffer->end();

        // set up Vulkan Synchronization 2 structures
        VkSemaphoreSubmitInfo waitSemaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = currentFrame.imageAvailableSemaphore,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
                         VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                         VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT
        };

        VkSemaphoreSubmitInfo signalSemaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = currentFrame.renderFinishedSemaphore,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
                         VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT
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

        m_currentFrame = (m_currentFrame + 1) % Renderer::Frame::MAX_FRAMES_IN_FLIGHT;
    }


    void Render::recreateSwapChain() {
        vkDeviceWaitIdle(m_context->device());

        // store old textures for deletion AFTER recreation
        std::vector<std::string> oldDepthKeys;
        for (size_t i = 0; i < m_frames.size(); ++i) {
            oldDepthKeys.push_back("Depth_Frame_" + std::to_string(i));
        }

        m_swapChain->recreate();

        // create new depth textures with unique names
        VkExtent2D extent = m_swapChain->extent();
        for (size_t i = 0; i < m_frames.size(); ++i) {
            auto &frame = m_frames[i];
            frame.commandBuffer = m_commandManager->createCommandBuffer();

            std::string newName = "Depth_Frame_" + std::to_string(i) + "_v" + std::to_string(m_swapchainVersion++);
            frame.depthTexture = &m_textureManager->createTexture(
                extent.width,
                extent.height,
                m_depthFormat->handle(),
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY,
                VK_IMAGE_ASPECT_DEPTH_BIT,
                false, true, newName
            );
        }

        for (auto &target: m_renderTargets) {
            target->recreateSwapChain();
        }

        m_renderGraphDebugPanel->invalidateCache();
    }

    void Render::markFramebufferResized() {
        m_framebufferResized = true;
    }


    void Render::cleanup() {
        /* Debugging VMA */
        char *StatsString = nullptr;
        vmaBuildStatsString(m_allocator, &StatsString, true);
        {
            std::ofstream OutStats{"VmaStats.json"};
            OutStats << StatsString;
        }
        vmaFreeStatsString(m_allocator, StatsString);
        for (auto &target: m_renderTargets) {
            target->cleanup();
        }
    }

    Render::~Render() {
        cleanup();
    }
}
