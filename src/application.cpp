#include "application.h"
#include <stdexcept>
#include <iostream>
#include "deletion_queue.h"

void VulkanApplication::run() {
    createWindowAndContext();
    createAllocator();

    // Create renderer and register its cleanup
    m_renderer = std::make_unique<Renderer>(m_context.get(), m_window.get(), m_allocator, MODEL_PATH, TEXTURE_PATH);

    // On window resize callback, notify renderer
    m_window->setResizeCallback([this]() {
        m_renderer->markFramebufferResized();
        });

    mainLoop();

    m_renderer.reset();


    DeletionQueue::get().flush();
    // Flush all deletions instead of manual cleanup
    //.flush();
}

void VulkanApplication::createWindowAndContext() {
    m_window = std::make_unique<Window>(WIDTH, HEIGHT, "Salamander");

    m_context = std::make_unique<Context>(m_window.get(), enableValidationLayers());
}

void VulkanApplication::createAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_context->physicalDevice();
    allocatorInfo.device = m_context->device();
    allocatorInfo.instance = m_context->instance();

    if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator!");
    }
    // Register allocator destruction
    DeletionQueue::get().pushFunction([this]() {
        vmaDestroyAllocator(m_allocator);
        });
}

void VulkanApplication::mainLoop() const {
    while (!m_window->shouldClose()) {
        m_window->pollEvents();
        try {
            m_renderer->drawFrame();
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Render error: " << e.what() << std::endl;
            m_renderer->recreateSwapChain();
        }
    }
    vkDeviceWaitIdle(m_context->device());
}