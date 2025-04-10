#include "window.h"
#include <stdexcept>

/* Initializes GLFW, sets a hint to avoid creating an OpenGL context (for Vulkan usage),
   and creates a GLFW window. Also registers the window pointer and resize callback. */
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

// Cleans up the GLFW window and terminates GLFW.
Window::~Window()
{
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

// Checks if the GLFW window should close (e.g., if the user has requested to close it).
bool Window::shouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

// Polls for GLFW events. Must be called regularly in the main loop so that window events are processed.
void Window::pollEvents() const
{
	glfwPollEvents();
}

/* Creates a Vulkan surface from the GLFW window using the given Vulkan instance.
   This surface is used later for swapchain creation and presentation. */
VkSurfaceKHR Window::createSurface(VkInstance instance) const
{
    VkSurfaceKHR surface;

	if (glfwCreateWindowSurface(instance, m_window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
	return surface;
}


/* Returns the underlying GLFWwindow* handle.
   Useful if lower-level access to the GLFW window is needed. */
GLFWwindow* Window::handle() const
{
	return m_window;
}

/* Retrieves the current framebuffer size as a VkExtent2D structure.
   This function queries GLFW for the current width and height. */
VkExtent2D Window::extent() const
{
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}


/* Blocks until the framebuffer has a non-zero size, and then returns it as VkExtent2D.
   This is useful when the window might be minimized or not yet visible. */
VkExtent2D Window::getValidExtent() const
{
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }
    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

/* Static callback function invoked by GLFW when the framebuffer is resized.
   It retrieves the Window instance from the GLFW window's user pointer and passes the new dimensions. */
void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    // retrieve the pointer set earlier in the constructor
    auto win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        win->onResize(width, height);
    }
}

/* Instance method that handles the resize event.
   Updates the stored width and height, and marks the window as resized. */
void Window::onResize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_resized = true;
}
