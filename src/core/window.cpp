#include "window.h"
#include <stdexcept>

#include "deletion_queue.h"

Window::Window(int width, int height, const char* title)
    : m_width(width), m_height(height), m_resized(false), m_resizeCallback(nullptr) {

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);

	DeletionQueue::get().pushFunction("Window", [this]() {
		if (m_window) {
			glfwDestroyWindow(m_window);
		}
        glfwTerminate();
		});

    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window!");
    }

    // Set up resize callback
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, Window::framebufferResizeCallback);
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
