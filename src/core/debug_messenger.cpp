#include "debug_messenger.h"
#include <iostream>
#include <stdexcept>

#include "../deletion_queue.h"

/* Stores the Vulkan instance and whether validation messages are enabled. If enabled,
   it sets up the debug messenger so that validation messages are forwarded to our callback. */
DebugMessenger::DebugMessenger(VkInstance instance, bool enableValidation)
    : m_instance(instance), m_enableValidation(enableValidation), m_messenger(VK_NULL_HANDLE)
{
    if (m_enableValidation) {
        setupDebugMessenger();
    }
}

/* Fills in the VkDebugUtilsMessengerCreateInfoEXT structure with the configuration wanted.
   This structure tells Vulkan which messages to report and what callback function to call. */
void DebugMessenger::populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    // set which severities to report: verbose, warning, and error
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    // set which types of messages to report: general, validation, and performance
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    // assign the callback function for handling debug messages
    createInfo.pfnUserCallback = debugCallback;
}

/* Sets up the debug messenger by querying Vulkan for the debug messenger creation function,
   then creating the messenger with the parameters set in populateCreateInfo(). */
void DebugMessenger::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateCreateInfo(createInfo);

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr && func(m_instance, &createInfo, nullptr, &m_messenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }

    /* If validation is enabled and a messenger was created, it retrieves the pointer to
   vkDestroyDebugUtilsMessengerEXT and uses it to clean up the messenger. */
	DeletionQueue::get().pushFunction([this]() {
		if (m_messenger != VK_NULL_HANDLE) {
			auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
			if (func != nullptr) {
				func(m_instance, m_messenger, nullptr);
			}
		}
		});
}

/* The static callback function that is invoked by Vulkan when a debug message is generated.
   It simply prints the message to std::cerr. Returning VK_FALSE tells Vulkan not to abort the call. */
VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}
