#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

#include "window.h"

// simple QueueFamilyIndices structure
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily; 

    bool isComplete() const {
        return graphicsFamily.has_value()  && presentFamily.has_value();
    }
};

class Context {
public:
    Context(Window* window, bool enableValidation);
    ~Context();
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;

    VkDevice device() const { return m_device; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    QueueFamilyIndices queueFamilies() const { return m_queueFamilies; }
    VkQueue graphicsQueue() const { return m_graphicsQueue; }
    VkQueue presentQueue() const { return m_presentQueue; }
    VkInstance instance() const { return m_instance; }
    VkSurfaceKHR surface() const { return m_surface; }

private:
    void createInstance();
    void createSurface(Window* window);
    void selectPhysicalDevice();
    void createLogicalDevice();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    static bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);
    std::vector<const char*> getRequiredExtensions(bool enableValidation);

    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    QueueFamilyIndices m_queueFamilies;

    bool m_enableValidation = false;

    // list of validation layers and device extensions.
    std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"

    };

    std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};