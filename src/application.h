//
// Created by ivans on 15/04/2025.
//

#ifndef SALAMANDER_APPLICATION_H
#define SALAMANDER_APPLICATION_H


#include "window.h"
#include "context.h"
#include "render.h"
#include "passes/depth_prepass.h"


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
    std::unique_ptr<Salamander::Core::Window> m_window;
    std::unique_ptr<Salamander::Core::Context> m_context;
    VmaAllocator m_allocator{ VK_NULL_HANDLE };
    std::unique_ptr<Salamander::Render> m_renderer;

    // Camera
    Salamander::Scene::Camera m_camera;

    float m_deltaTime = 0.0f;
    float m_lastFrameTime = 0.0f;
};


#endif //SALAMANDER_APPLICATION_H

