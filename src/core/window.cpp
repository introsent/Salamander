#include "window.h"

#include <stdexcept>

#include "deletion_queue.h"
#include "camera/camera.h"

Window::Window(int width, int height, const char* title)
    : m_width(width), m_height(height) {

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    DeletionQueue::get().pushFunction("Window", [this]() {
        if (m_window) glfwDestroyWindow(m_window);
        glfwTerminate();
    });

    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window!");
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, Window::framebufferResizeCallback);
}

void Window::setupInputCallbacks() {
    glfwSetMouseButtonCallback(m_window, Window::mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos) {
        Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        self->handleMouseMovement(xpos, ypos);
    });
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        self->m_leftMousePressed = (action == GLFW_PRESS);
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        self->m_rightMousePressed = (action == GLFW_PRESS);
    }
}

void Window::handleMouseMovement(double xpos, double ypos) {
    if (!m_camera) return;

    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    double xoffset = xpos - m_lastX;
    double yoffset = m_lastY - ypos; // Reversed y-coordinates
    m_lastX = xpos;
    m_lastY = ypos;

    if (m_leftMousePressed) {
        if (m_rightMousePressed) {
            // Both buttons pressed - vertical movement
            m_camera->ProcessVerticalMovement(static_cast<float>(yoffset));
        } else {
            // Only left button pressed - rotation
            m_camera->ProcessMouseMovement(static_cast<float>(xoffset),
                                          static_cast<float>(yoffset));
        }
    }
    if (m_rightMousePressed) {
        if (!m_leftMousePressed)
        {
            m_camera->ProcessHorizontalMovement(static_cast<float>(yoffset));
        }

    }
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto* win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        win->handleResize(width, height);
    }
}

void Window::handleResize(int width, int height) {
    m_width = width;
    m_height = height;
    m_resized = true;

    if (m_resizeCallback) {
        m_resizeCallback();
    }
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents() const
{
    glfwPollEvents();
}

VkSurfaceKHR Window::createSurface(VkInstance instance) const
{
    VkSurfaceKHR surface;

    if (glfwCreateWindowSurface(instance, m_window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    return surface;
}

VkExtent2D Window::extent() const
{
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

VkExtent2D Window::getValidExtent() const
{
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }
    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}