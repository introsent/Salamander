#pragma once

#define GLFW_INCLUDE_VULKAN
#include <functional>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "camera/camera.h"

class Camera;

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window() = default;

    bool shouldClose() const;
    void pollEvents() const;
    VkSurfaceKHR createSurface(VkInstance instance) const;
    VkExtent2D extent() const;
    VkExtent2D getValidExtent() const;

    void setResizeCallback(std::function<void()> callback) {
        m_resizeCallback = callback;
    }

    void setupInputCallbacks();
    void setCamera(Camera* camera) {  m_camera = camera;}
    GLFWwindow* handle() const { return m_window; }

private:
    void handleResize(int width, int height);

    GLFWwindow* m_window;
    int m_width;
    int m_height;
    bool m_resized = false;
    std::function<void()> m_resizeCallback;

    Camera* m_camera = nullptr;

    // Mouse state
    bool m_leftMousePressed = false;
    bool m_rightMousePressed = false;
    double m_lastX = 0.0;
    double m_lastY = 0.0;
    bool m_firstMouse = true;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    void handleMouseMovement(double xpos, double ypos);
};
