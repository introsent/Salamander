#include "application.h"
#include <iostream>

void VulkanApplication::run()
{
    createWindowAndContext();
    createAllocator();
    m_renderer = new Renderer(m_context, m_window, allocator, MODEL_PATH, TEXTURE_PATH);
    m_window->setResizeCallback([this]() {
        m_renderer->markFramebufferResized();
        });
    mainLoop();
    cleanup();
}

void VulkanApplication::createWindowAndContext()
{
    m_window = new Window(WIDTH, HEIGHT, "Salamander");
    m_context = new Context(m_window, enableValidationLayers());
}

void VulkanApplication::createAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_context->physicalDevice();
    allocatorInfo.device = m_context->device();
    allocatorInfo.instance = m_context->instance();

    if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator!");
    }
}

void VulkanApplication::mainLoop() const
{
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

void VulkanApplication::cleanup() const
{
    delete m_renderer;
    vmaDestroyAllocator(allocator);
    delete m_context;
    delete m_window;
}


