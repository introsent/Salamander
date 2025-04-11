#pragma once
#include "context.h"
#include "swap_chain.h"
#include <vector>

class ImageViews final {
public:
    ImageViews(Context* context, const SwapChain* swapChain);
    ~ImageViews();
    ImageViews(const ImageViews&) = delete;
    ImageViews& operator=(const ImageViews&) = delete;
    ImageViews(ImageViews&&) = delete;
    ImageViews& operator=(ImageViews&&) = delete;

    const std::vector<VkImageView>& views() const { return m_imageViews; }
    void recreate(const SwapChain* swapChain);

private:
    void createImageViews(const SwapChain* swapChain);
    void cleanup();

    Context* m_context;
    std::vector<VkImageView> m_imageViews;
    VkFormat m_format;
};
