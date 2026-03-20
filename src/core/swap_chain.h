// swap_chain.h
#pragma once
#include <memory>

#include "context.h"
#include <vector>

#include "image_views.h"


namespace Salamander::Core {
    class SwapChain final {
    public:
        SwapChain(Context * context, Window * window);
        SwapChain(const SwapChain &) = delete;
        SwapChain & operator=(const SwapChain &) = delete;
        SwapChain(SwapChain &&) = delete;
        SwapChain & operator=(SwapChain &&) = delete;

        void recreate();
        [[nodiscard]] VkSwapchainKHR handle() const { return m_swapChain; }

        [[nodiscard]] VkExtent2D extent() const { return m_extent; }
        [[nodiscard]] VkFormat format() const { return m_imageFormat; }
        [[nodiscard]] const std::vector<VkImage> & images() const { return m_images; }
        [[nodiscard]] std::vector<VkImageView> imagesViews() const { return m_imageViews->views(); }
        [[nodiscard]] VkImage getCurrentImage(uint32_t imageIndex) const { return m_images[imageIndex]; }


    private:
        void createSwapChain();
        void createImageViews();
        void cleanup();

        static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;

        Context *m_context;
        Window *m_window;
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
        VkFormat m_imageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_extent{};
        std::vector<VkImage> m_images;
        std::unique_ptr<ImageViews> m_imageViews;
    };
}
