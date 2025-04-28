#pragma once

#define GLFW_INCLUDE_VULKAN
#include <functional>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

class Window final {
public:
    Window(int width, int height, const char* title);

    // Delete copy and move operations
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    // Window operations
    bool shouldClose() const;
    void pollEvents() const;
    VkSurfaceKHR createSurface(VkInstance instance) const;

    // Accessors
    GLFWwindow* handle() const { return m_window; }
    VkExtent2D extent() const;
    VkExtent2D getValidExtent() const;
    bool isResized() const { return m_resized; }
    void resetResizedFlag() { m_resized = false; }

    // Callback management
    void setResizeCallback(std::function<void()> callback) {
        m_resizeCallback = callback;
    }

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void handleResize(int width, int height);

    GLFWwindow* m_window;
    int m_width;
    int m_height;
    bool m_resized;
    std::function<void()> m_resizeCallback;
};