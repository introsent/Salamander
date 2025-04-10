#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class DebugMessenger final {
public:
    DebugMessenger(VkInstance instance, bool enableValidation);
    ~DebugMessenger();
    DebugMessenger(const DebugMessenger&) = delete;
    DebugMessenger& operator=(const DebugMessenger&) = delete;
    DebugMessenger(DebugMessenger&&) = delete;
    DebugMessenger& operator=(DebugMessenger&&) = delete;

    VkDebugUtilsMessengerEXT messenger() const { return m_messenger; }

private:
    static void populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_messenger;
    bool m_enableValidation;
};