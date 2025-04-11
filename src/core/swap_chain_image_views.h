#pragma once
#include "context.h"
#include "swap_chain.h"
#include <vector>

class SwapChainImageViews final {
public:
    SwapChainImageViews(Context* context, const SwapChain* swapChain);
    ~SwapChainImageViews();

    const std::vector<VkImageView>& views() const { return m_imageViews; }
    void recreate(const SwapChain* swapChain);

private:
    void createImageViews(const SwapChain* swapChain);
    void cleanup();

    Context* m_context;
    std::vector<VkImageView> m_imageViews;
    VkFormat m_format;
};
