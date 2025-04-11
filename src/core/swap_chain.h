// swap_chain.h
#pragma once
#include "context.h"
#include <vector>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class SwapChain final {
public:
    SwapChain(Context* context, Window* window);
    ~SwapChain();

    void recreate();
    VkSwapchainKHR swapChain() const { return m_swapChain; }
    VkExtent2D extent() const { return m_extent; }
    VkFormat format() const { return m_imageFormat; }
    const std::vector<VkFramebuffer>& framebuffers() const { return m_framebuffers; }
    void createFramebuffers(VkRenderPass renderPass, VkImageView depthImageView);
    SwapChainSupportDetails querySwapChainSupport() const;

private:
    void createSwapChain();
    void createImageViews();
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
    std::vector<VkImageView> m_imageViews;
    std::vector<VkFramebuffer> m_framebuffers;
};