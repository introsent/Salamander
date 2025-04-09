#include "window.h"
#include <stdexcept>

Window::Window(int width, int height, const char* title) : m_width(width), m_height(height), m_resized(false)
{
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW!");
    }

    // ensure GLFW does not create an OpenGL context:
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // optionally disable window resizing
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(m_width, m_height, title, nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window!");
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, Window::framebufferResizeCallback);
}

Window::~Window()
{
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

// check if the window should close
bool Window::shouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

// poll events using GLFW
void Window::pollEvents() const
{
	glfwPollEvents();
}

// create a vulkan surface tied to this window
VkSurfaceKHR Window::createSurface(VkInstance instance) const
{
    VkSurfaceKHR surface;

	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
	return surface;
}

GLFWwindow* Window::handle() const
{
	return m_window;
}

VkExtent2D Window::extent() const
{
	return { m_width, m_height };
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    // retrieve the pointer set earlier in the constructor
    auto win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        win->onResize(width, height);
    }
}

// instance method to process a resize event, update state accordingly
void Window::onResize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_resized = true;
}
