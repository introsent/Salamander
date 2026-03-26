//
// Created by ivans on 11/04/2025.
//

#ifndef SALAMANDER_FRAMEBUFFER_MANAGER_H
#define SALAMANDER_FRAMEBUFFER_MANAGER_H


#include "context.h"
#include <vector>
#include <unordered_map>

namespace Salamander::Core {
    class FramebufferManager final {
    public:
        struct FramebufferSet {
            std::vector<VkFramebuffer> framebuffers;
        };

        FramebufferManager(Context * context,

        const std::vector<VkImageView> *imageViews,
        VkExtent2D extent,
        VkImageView depthImageView
        )
        ;

        FramebufferManager(const FramebufferManager &) = delete;
        FramebufferManager & operator=(const FramebufferManager &) = delete;
        FramebufferManager(FramebufferManager &&) = delete;
        FramebufferManager & operator=(FramebufferManager &&) = delete;

        FramebufferSet createFramebuffersForRenderPass(VkRenderPass renderPass);
        void recreate(const std::vector<VkImageView> *imageViews,
                      VkExtent2D extent,
                      VkImageView depthImageView);

    private:
        void cleanup();

        Context *m_context;
        const std::vector<VkImageView> *m_imageViews;
        VkExtent2D m_extent;
        VkImageView m_depthView;
        std::unordered_map<VkRenderPass, FramebufferSet> m_framebufferSets;
    };
}


#endif //SALAMANDER_FRAMEBUFFER_MANAGER_H