#include "application.h"
#include <stdexcept>
#include <iostream>

void VulkanApplication::run() {
    createWindowAndContext();
    createAllocator();

    // Create renderer and register its cleanup
    m_renderer = new Renderer(m_context, m_window, m_allocator, MODEL_PATH, TEXTURE_PATH);
    m_deletionQueue.push([this]() {
        delete m_renderer;
        });

    // On window resize callback, notify renderer
    m_window->setResizeCallback([this]() {
        m_renderer->markFramebufferResized();
        });

    mainLoop();
    // Flush all deletions instead of manual cleanup
    m_deletionQueue.flush();
}

void VulkanApplication::createWindowAndContext() {
    m_window = new Window(WIDTH, HEIGHT, "Salamander");
    // Register window deletion
    m_deletionQueue.push([this]() {
        delete m_window;
        });

    m_context = new Context(m_window, enableValidationLayers());
    // Register context deletion
    m_deletionQueue.push([this]() {
        delete m_context;
        });
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
    m_deletionQueue.push([this]() {
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