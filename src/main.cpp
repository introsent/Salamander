#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <string>

#include "core/data_structures.h"
#include "core/window.h"
#include "core/context.h"
#include "renderer.h"

// Application constants
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
const std::string MODEL_PATH = "./models/viking_room.obj";
const std::string TEXTURE_PATH = "./textures/viking_room.png";

class VulkanApplication {
public:
    void run() {
        createWindowAndContext();
        createAllocator();
        m_renderer = new Renderer(m_context, m_window, allocator, MODEL_PATH, TEXTURE_PATH);
        m_window->setResizeCallback([this]() {
            m_renderer->markFramebufferResized();
            });
        mainLoop();
        cleanup();
    }

private:
    Window* m_window = nullptr;
    Context* m_context = nullptr;
    Renderer* m_renderer = nullptr;
    VmaAllocator allocator;

    static bool enableValidationLayers() {
#ifdef NDEBUG
        return false;
#else
        return true;
#endif
    }

    void createWindowAndContext() {
        m_window = new Window(WIDTH, HEIGHT, "Salamander");
        m_context = new Context(m_window, enableValidationLayers());
    }

    void createAllocator() {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = m_context->physicalDevice();
        allocatorInfo.device = m_context->device();
        allocatorInfo.instance = m_context->instance();

        if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create VMA allocator!");
        }
    }

    void mainLoop() {
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

    void cleanup() {
        delete m_renderer;
        vmaDestroyAllocator(allocator);
        delete m_context;
        delete m_window;
    }
};

int main() {
    VulkanApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
