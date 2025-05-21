#pragma once
#include "render_pass_executor.h"
#include <vector>
#include "data_structures.h"
#include "ssbo_buffer.h"
#include "swap_chain.h"

class MainSceneExecutor : public RenderPassExecutor {
public:
    struct Resources {
        VkDevice                        device;
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
        std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> depthImageViews;
        std::array<VkImage, MAX_FRAMES_IN_FLIGHT> depthImages;
        VkImage                         gBufferAlbedoImage;
        VkImageView                     gBufferAlbedoView;
        VkImage                         gBufferNormalImage;
        VkImageView                     gBufferNormalView;
        VkImage                         gBufferParamsImage;
        VkImageView                     gBufferParamsView;
        VkImage                         depthImage;
        VkImageView                     depthView;
        VkClearValue                    clearColor;
        VkClearValue                    clearDepth;
        SwapChain*                      swapChain;
        std::vector<GLTFPrimitiveData>  primitives;
        uint32_t*                       currentFrame;
        uint32_t                        textureCount;
        VkImage                         hdrImage;
        VkImageView                     hdrImageView;
        std::vector<VkDescriptorSet>    toneDescriptorSets;
        VkPipeline                      tonePipeline;
        VkPipelineLayout                tonePipelineLayout;
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
    std::array<VkImageLayout, MAX_FRAMES_IN_FLIGHT> m_currentDepthLayouts;

};