#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class DebugMessenger final {
public:
    DebugMessenger(VkInstance instance, bool enableValidation);
    DebugMessenger(const DebugMessenger&) = delete;
    DebugMessenger& operator=(const DebugMessenger&) = delete;
    DebugMessenger(DebugMessenger&&) = delete;
    DebugMessenger& operator=(DebugMessenger&&) = delete;

    VkDebugUtilsMessengerEXT messenger() const { return m_messenger; }

    void setupDeviceFunctions(VkDevice device);

    void setObjectName(uint64_t objectHandle, VkObjectType objectType, const char* name) const;
    void beginCmdDebugLabel(VkCommandBuffer commandBuffer, const char* labelName, const float color[4]) const;
    void endCmdDebugLabel(VkCommandBuffer commandBuffer) const;
    void insertCmdDebugLabel(VkCommandBuffer commandBuffer, const char* labelName, const float color[4]) const;

private:
    static void populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    VkInstance m_instance;
    VkDevice m_device = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_messenger;
    bool m_enableValidation;

    // Function pointers for debug utilities
    PFN_vkSetDebugUtilsObjectNameEXT m_setDebugUtilsObjectName = nullptr;
    PFN_vkCmdBeginDebugUtilsLabelEXT m_cmdBeginDebugUtilsLabel = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT m_cmdEndDebugUtilsLabel = nullptr;
    PFN_vkCmdInsertDebugUtilsLabelEXT m_cmdInsertDebugUtilsLabel = nullptr;
};