#pragma once
#include "context.h"
#include <vector>

class ImageViews final {
public:
    ImageViews(Context* context, VkFormat format, const std::vector<VkImage>& images);
    ImageViews(const ImageViews&) = delete;
    ImageViews& operator=(const ImageViews&) = delete;
    ImageViews(ImageViews&&) = delete;
    ImageViews& operator=(ImageViews&&) = delete;

    const std::vector<VkImageView>& views() const { return m_imageViews; }
    void recreate(VkFormat format, const std::vector<VkImage>& images);

private:
    void createImageViews(const std::vector<VkImage>& images);
    void cleanup();

    Context* m_context;
    std::vector<VkImageView> m_imageViews;
    VkFormat m_format;
};
