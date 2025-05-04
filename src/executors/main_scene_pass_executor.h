#pragma once
#include "render_pass_executor.h"
#include <vector>
#include <vulkan/vulkan.h>

class RenderPass;
class MainScenePassExecutor : public RenderPassExecutor
{
public:
    struct Resources {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkBuffer vertexBuffer;
        VkBuffer indexBuffer;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<uint32_t> indices;
        std::vector<VkFramebuffer>& framebuffers;
        VkExtent2D extent;
    };

    MainScenePassExecutor(RenderPass* renderPass, Resources resources);

    void begin(VkCommandBuffer cmd, uint32_t imageIndex) override;
    void execute(VkCommandBuffer cmd) override;
    void end(VkCommandBuffer cmd) override;

private:
    void setViewportAndScissor(VkCommandBuffer cmd) const;

    void bindBuffers(VkCommandBuffer cmd) const;

    RenderPass* m_renderPass;
    Resources m_resources;

};

