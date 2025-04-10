#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

// simple QueueFamilyIndices structure
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily; 

    bool isComplete() const {
        return graphicsFamily.has_value() /* && presentFamily.has_value() if required in teh future */;
    }
};

class Context {
public:
   Context(bool enableValidation);
    ~Context();
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;

    VkDevice device() const { return m_device; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    QueueFamilyIndices queueFamilies() const { return m_queueFamilies; }
    VkInstance instance() const { return m_instance; }

private:
    void createInstance();
    void selectPhysicalDevice();
    void createLogicalDevice();

    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    QueueFamilyIndices m_queueFamilies;

    bool m_enableValidation = false;

    // list of validation layers and device extensions.
    std::vector<const char*> m_validationLayers = {
#ifndef NDEBUG
        "VK_LAYER_KHRONOS_validation"
#endif
    };

    std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};