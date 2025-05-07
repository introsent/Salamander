#pragma once
#include "data_structures.h"
#include "window.h"
#include "context.h"
#include "renderer.h"


constexpr uint32_t WIDTH = 1100;
constexpr uint32_t HEIGHT = 900;

class VulkanApplication {
public:
    void run();
private:
    void createWindowAndContext();
    void createAllocator();
    void mainLoop();

    static bool enableValidationLayers() {
#ifdef NDEBUG
        return false;
#else
        return true;
#endif
    }

    // Resources
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Context> m_context;
    VmaAllocator m_allocator{ VK_NULL_HANDLE };
    std::unique_ptr<Renderer> m_renderer;

    // Camera
    Camera m_camera;

    float m_deltaTime = 0.0f;
    float m_lastFrameTime = 0.0f;

};

