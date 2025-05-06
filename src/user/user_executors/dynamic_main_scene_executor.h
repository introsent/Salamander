#pragma once
#include "render_pass_executor.h"
#include <vector>
#include "swap_chain.h"

class DynamicMainSceneExecutor : public RenderPassExecutor {
public:
    struct Resources {
        VkPipeline                      pipeline;
        VkPipelineLayout                pipelineLayout;
        VkBuffer                        vertexBuffer;
        VkBuffer                        indexBuffer;
        std::vector<VkDescriptorSet>    descriptorSets;
        std::vector<uint32_t>           indices;
        VkExtent2D                      extent;
        std::vector<VkImageView>        colorImageViews;
        VkImageView                     depthImageView;
        VkClearValue                    clearColor;
        VkClearValue                    clearDepth;
        SwapChain*                      swapChain;
    };

    explicit DynamicMainSceneExecutor(Resources resources);

    void begin(VkCommandBuffer cmd, uint32_t imageIndex) override;
    void execute(VkCommandBuffer cmd) override;
    void end(VkCommandBuffer cmd) override;

    void updateResources(SwapChain* swapChain, VkImageView depthView);


private:
    void setViewportAndScissor(VkCommandBuffer cmd) const;
    void bindBuffers(VkCommandBuffer cmd) const;

    Resources m_resources;
    VkRenderingAttachmentInfo m_colorAttachment{};
    VkRenderingAttachmentInfo m_depthAttachment{};
    uint32_t m_currentImageIndex{};
};