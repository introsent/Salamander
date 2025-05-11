#include "application.h"
#include <stdexcept>
#include <iostream>
#include "deletion_queue.h"

void VulkanApplication::run() {
    createWindowAndContext();

    // Setup camera
    m_window->setCamera(&m_camera); //reference camera
    m_window->setupInputCallbacks();

    // Initialize camera with starting position
    m_camera = Camera(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f, -90.f);

    createAllocator();

    // Create renderer and register its cleanup
    m_renderer = std::make_unique<Renderer>(m_context.get(), m_window.get(), m_allocator, &m_camera);

    // On window resize callback, notify renderer
    m_window->setResizeCallback([this]() {
        m_renderer->markFramebufferResized();
        });

    mainLoop();

    m_renderer.reset();

    // Flush all deletions instead of manual cleanup
    DeletionQueue::get().flush();
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
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator!");
    }
    // Register allocator destruction
    DeletionQueue::get().pushFunction("Allocator", [this]() {
        vmaDestroyAllocator(m_allocator);
        });
}

void VulkanApplication::mainLoop() {
    while (!m_window->shouldClose()) {
        float currentFrame = glfwGetTime();  // GLFW's high-resolution timer
        m_deltaTime = currentFrame - m_lastFrameTime;
        m_lastFrameTime = currentFrame;

        m_window->pollEvents();
        try {
            m_camera.ProcessKeyboard(m_window->handle() , m_deltaTime);
            m_renderer->drawFrame();
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Render error: " << e.what() << std::endl;
            m_renderer->recreateSwapChain();
        }
    }
    vkDeviceWaitIdle(m_context->device());
}
