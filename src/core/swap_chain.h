// swap_chain.h
#pragma once
#include "context.h"
#include <vector>


class SwapChain final {
public:
    SwapChain(Context* context, Window* window);
    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;
    SwapChain(SwapChain&&) = delete;
    SwapChain& operator=(SwapChain&&) = delete;

    void recreate();
    VkSwapchainKHR handle() const { return m_swapChain; }
    VkExtent2D extent() const { return m_extent; }
    VkFormat format() const { return m_imageFormat; }
    const std::vector<VkImage>& images() const { return m_images; }

private:
    void createSwapChain();
    void cleanup();

    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    Context* m_context;
    Window* m_window;
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkFormat m_imageFormat;
    VkExtent2D m_extent;
    std::vector<VkImage> m_images;
};