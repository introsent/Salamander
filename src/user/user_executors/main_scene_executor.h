#pragma once
#include "render_pass_executor.h"
#include <vector>
#include "data_structures.h"
#include "ssbo_buffer.h"
#include "swap_chain.h"

class MainSceneExecutor : public RenderPassExecutor {
public:
    struct Resources {
        VkPipeline                      lightingPipeline;        // For lighting pass
        VkPipelineLayout                lightingPipelineLayout;
        VkPipeline                      depthPipeline;          // For depth pre-pass
        VkPipelineLayout                depthPipelineLayout;
        VkPipeline                      gBufferPipeline;        // For G-buffer pass
        VkPipelineLayout                gBufferPipelineLayout;
        uint64_t                        vertexBufferAddress;
        VkBuffer                        indexBuffer;
        std::vector<VkDescriptorSet>    gBufferDescriptorSets;  // For depth and G-buffer passes
        std::vector<VkDescriptorSet>    lightingDescriptorSets; // For lighting pass
        std::vector<uint32_t>           indices;
        VkExtent2D                      extent;
        std::vector<VkImageView>        colorImageViews;
        VkImageView                     depthImageView;
        VkImage                         depthImage;
        VkImage                         gBufferAlbedoImage;
        VkImageView                     gBufferAlbedoView;
        VkImage                         gBufferNormalImage;
        VkImageView                     gBufferNormalView;
        VkImage                         gBufferParamsImage;
        VkImageView                     gBufferParamsView;
        VkClearValue                    clearColor;
        VkClearValue                    clearDepth;
        SwapChain*                      swapChain;
        std::vector<GLTFPrimitiveData>  primitives;
        uint32_t                        currentFrame;
    };

    explicit MainSceneExecutor(Resources resources);

    void begin(VkCommandBuffer cmd, uint32_t imageIndex) override;
    void execute(VkCommandBuffer cmd) override;
    void end(VkCommandBuffer cmd) override;

private:
    void setViewportAndScissor(VkCommandBuffer cmd) const;
    void bindBuffers(VkCommandBuffer cmd, VkPipelineLayout layout, const std::vector<VkDescriptorSet>& descriptorSets) const;
    void drawPrimitives(VkCommandBuffer cmd, VkPipelineLayout layout) const;

    Resources m_resources;
    VkRenderingAttachmentInfo m_colorAttachment{};
    VkRenderingAttachmentInfo m_depthAttachment{};
    uint32_t m_currentImageIndex{};
};