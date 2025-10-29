#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

#include "window.h"
#include "debug_messenger.h"

// Since user can use different vulkan version, added alterative KHR extensions
struct SupportedDeviceFeatures {
    VkPhysicalDeviceFeatures2 coreFeatures{};
    VkPhysicalDeviceVulkan12Features features12{};
    VkPhysicalDeviceVulkan13Features features13{};

    // Alternative: KHR extensions for Vulkan < 1.3
    VkPhysicalDeviceSynchronization2FeaturesKHR sync2FeaturesKHR{};
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKHR{};

    bool usingVulkan13 = false; // Track which path we're using
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class Context final {
public:
    Context(Window* window, bool enableValidation);
    ~Context();
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

    VkDevice device() const { return m_device; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    QueueFamilyIndices queueFamilies() const { return m_queueFamilies; }
    VkQueue graphicsQueue() const { return m_graphicsQueue; }
    VkQueue presentQueue() const { return m_presentQueue; }
    VkInstance instance() const { return m_instance; }
    VkSurfaceKHR surface() const { return m_surface; }

    DebugMessenger* debugMessenger() const { return m_debugMessenger; }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

private:
    void createInstance();
    void createSurface(Window* window);
    void selectPhysicalDevice();
    void createLogicalDevice();

    bool checkValidationLayerSupport() const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
    bool isDeviceSuitable(VkPhysicalDevice device) const;

    // Query and store device features
    static SupportedDeviceFeatures queryDeviceFeatures(VkPhysicalDevice device) ;

    // Validate that required features are supported
    static bool validateRequiredFeatures(const SupportedDeviceFeatures& features);

    static std::vector<const char*> getRequiredInstanceExtensions(bool enableValidation);

    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    QueueFamilyIndices m_queueFamilies;
    DebugMessenger* m_debugMessenger;
    bool m_enableValidation = false;


    SupportedDeviceFeatures m_supportedFeatures {};

    std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME
    };
};