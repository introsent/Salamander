#include "context.h"
#include <iostream>
#include <vector>
#include <set>
#include <cstring>
#include "deletion_queue.h"

// Checks if all the requested validation layers are available on the system
bool Context::checkValidationLayerSupport() const
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const char* layerName : m_validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

bool Context::checkDeviceExtensionSupport(VkPhysicalDevice device) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}

bool Context::isDeviceSuitable(VkPhysicalDevice device) const {
    // Check queue families
    QueueFamilyIndices indices = findQueueFamilies(device);
    if (!indices.isComplete()) {
        return false;
    }

    // Check device extensions
    if (!checkDeviceExtensionSupport(device)) {
        return false;
    }

    // Check swap chain support
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    bool swapChainAdequate = !swapChainSupport.formats.empty() &&
                             !swapChainSupport.presentModes.empty();
    if (!swapChainAdequate) {
        return false;
    }

    // Query and validate features
    SupportedDeviceFeatures features = queryDeviceFeatures(device);
    return validateRequiredFeatures(features);
}

SupportedDeviceFeatures Context::queryDeviceFeatures(VkPhysicalDevice device)
{
    SupportedDeviceFeatures features;

    // Get device properties to check Vulkan version
    VkPhysicalDeviceProperties deviceProps;
    vkGetPhysicalDeviceProperties(device, &deviceProps);

    // Determine if we're using Vulkan 1.3 core or KHR extensions
    features.usingVulkan13 = (deviceProps.apiVersion >= VK_MAKE_VERSION(1, 3, 0));

    // Initialize core features structure (always first in chain despite the Vulkan version)
    features.coreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    // Path for Vulkan >= 1.3
    if (features.usingVulkan13) {
        // Initialize Vulkan 1.2 features
        features.features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

        // Initialize Vulkan 1.3 features
        features.features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        // Chain: coreFeatures -> features12 -> features13
        features.features12.pNext = &features.features13;
        features.coreFeatures.pNext = &features.features12;

    // Path for Vulkan < 1.3
    } else {
        // Initialize Vulkan 1.2 features
        features.features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

        // Initialize KHR extension features
        features.sync2FeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
        features.dynamicRenderingFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;

        // Chain: coreFeatures -> features12 -> sync2 -> dynamicRendering
        features.sync2FeaturesKHR.pNext = &features.dynamicRenderingFeaturesKHR;
        features.features12.pNext = &features.sync2FeaturesKHR;
        features.coreFeatures.pNext = &features.features12;
    }

    // Query all features at once using the chained structure
    vkGetPhysicalDeviceFeatures2(device, &features.coreFeatures);

    return features;
}

bool Context::validateRequiredFeatures(const SupportedDeviceFeatures& features)
{
    // Check core feature (not in one IF statement for better debugging)
    if (features.coreFeatures.features.samplerAnisotropy != VK_TRUE)
    {
        return false;
    }
    if (features.coreFeatures.features.shaderInt64 != VK_TRUE)
    {
        return false;
    }

    // Check Vulkan 1.2 features
    if (features.features12.bufferDeviceAddress != VK_TRUE)
    {
        return false;
    }
    if (features.features12.runtimeDescriptorArray != VK_TRUE)
    {
        return false;
    }
    if (features.features12.scalarBlockLayout != VK_TRUE)
    {
        return false;
    }

    // Check synchornization2 and dynamic rendering
    // for Vulkan >= 1.3
    if (features.usingVulkan13)
    {
        if (features.features13.synchronization2 != VK_TRUE)
        {
            return false;
        }
        if (features.features13.dynamicRendering != VK_TRUE)
        {
            return false;
        }
    }
    // for Vulkan < 1.3
    else
    {
        if (features.sync2FeaturesKHR.synchronization2 != VK_TRUE)
        {
            return false;
        }
        if (features.dynamicRenderingFeaturesKHR.dynamicRendering != VK_TRUE)
        {
            return false;
        }
    }

    // if non-failed - validation is passed
    return true;
}

std::vector<const char*> Context::getRequiredInstanceExtensions(bool enableValidation) {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = nullptr;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

/* Initializes the Vulkan instance, debug messenger,
   creates the surface from the provided Window, selects a physical device,
   creates the logical device. */
Context::Context(Window* window, bool enableValidation)
    : m_enableValidation(enableValidation)
{
    createInstance();
    m_debugMessenger = new DebugMessenger(m_instance, enableValidation);
    createSurface(window);
    selectPhysicalDevice();
    createLogicalDevice();
    m_debugMessenger->setupDeviceFunctions(m_device);
}

// cleans up all Vulkan resources
Context::~Context() {
    delete m_debugMessenger;
}

/* Creates the Vulkan instance using the information set in the VkApplicationInfo and required extensions.
   Validates that requested validation layers are available. */
void Context::createInstance() {
    if (m_enableValidation && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanContextApp";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    auto extensions = getRequiredInstanceExtensions(m_enableValidation);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (m_enableValidation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
	DeletionQueue::get().pushFunction("Instance", [this]() {
		if (m_instance != VK_NULL_HANDLE) {
			vkDestroyInstance(m_instance, nullptr);
		}
		});
}

// Creates the window surface using the provided Window object's createSurface method.
void Context::createSurface(Window* window)
{
    m_surface = window->createSurface(m_instance);
    if (m_surface == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to create window surface!");
    }

    DeletionQueue::get().pushFunction("Surface", [this]() {
        if (m_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }
        });
}

/* Selects a physical device (GPU) that supports the necessary queues and extensions.
   This function uses the m_surface (created earlier) to check for presentation support.*/
void Context::selectPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            m_physicalDevice = device;
            m_queueFamilies = findQueueFamilies(device);
            m_supportedFeatures = queryDeviceFeatures(device);
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}



SwapChainSupportDetails Context::querySwapChainSupport(VkPhysicalDevice device) const {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

/* Creates a logical device from the selected physical device.
   Also retrieves the graphics and presentation queues from the created device. */
void Context::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                             indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (auto queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Enable already queried features

    VkPhysicalDeviceFeatures2 enabledFeatures{};
    enabledFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    // Enable core features
    enabledFeatures.features.samplerAnisotropy = VK_TRUE;
    enabledFeatures.features.shaderInt64 = VK_TRUE;

    // Vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features enabled12{};
    enabled12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    enabled12.bufferDeviceAddress = VK_TRUE;
    enabled12.runtimeDescriptorArray = VK_TRUE;
    enabled12.scalarBlockLayout = VK_TRUE;

    void* pNext = &enabled12; // Start building the chain

    // Create variables before IF statement to avoid dangling pointer
    VkPhysicalDeviceVulkan13Features enabled13{};
    VkPhysicalDeviceSynchronization2FeaturesKHR enabledSync2{};
    VkPhysicalDeviceDynamicRenderingFeaturesKHR enabledDynRendering{};

    if (m_supportedFeatures.usingVulkan13) {
        // Vulkan 1.3 path
        enabled13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        enabled13.synchronization2 = VK_TRUE;
        enabled13.dynamicRendering = VK_TRUE;

        // Chain: enabled12 -> enabled13
        enabled12.pNext = &enabled13;
    } else {
        // KHR extension path
        enabledSync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
        enabledSync2.synchronization2 = VK_TRUE;

        enabledDynRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
        enabledDynRendering.dynamicRendering = VK_TRUE;

        // Chain: enabled12 -> enabledSync2 -> enabledDynRendering
        enabledSync2.pNext = &enabledDynRendering;
        enabled12.pNext = &enabledSync2;
    }

    // Attach the entire chain to enabledFeatures
    enabledFeatures.pNext = pNext;

    // Create device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pNext = &enabledFeatures; // Attach our feature chain
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

    if (m_enableValidation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    DeletionQueue::get().pushFunction("Device", [this]() {
        if (m_device != VK_NULL_HANDLE) {
            vkDestroyDevice(m_device, nullptr);
        }
    });

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}


// Queries the available queue families for the physical device and returns the ones that support both graphics and presentation.
QueueFamilyIndices Context::findQueueFamilies(VkPhysicalDevice device) const {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;
        if (presentSupport)
            indices.presentFamily = i;
        if (indices.isComplete())
            break;
        i++;
    }
    return indices;
}


