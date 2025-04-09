#pragma once

#include <vulkan.h>
#include <GLFW/glfw3.h>

class Window final {
public:
    Window(int width, int height, const char* title);
    ~Window();

    bool shouldClose() const;
    void pollEvents() const;
    VkSurfaceKHR createSurface(VkInstance instance) const;

    // Getters
    GLFWwindow* handle() const;
    VkExtent2D extent() const;

    bool isResized() const { return m_resized; }
    void resetResizedFlag() { m_resized = false; }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
    void onResize(int width, int height);

    GLFWwindow* m_window;
    int m_width, m_height;
    bool m_resized;
};