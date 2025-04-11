#pragma once
#include <vector>
#include <string>
#include <vk_mem_alloc.h>
#include "../core/context.h"

struct PipelineConfig {
    std::string vertShaderPath;
    std::string fragShaderPath;
    VkVertexInputBindingDescription bindingDescription;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineColorBlendStateCreateInfo colorBlending;
    VkPipelineDynamicStateCreateInfo dynamicState;
};

class Pipeline {
public:
    Pipeline(
        Context* context,
        VkRenderPass renderPass,
        VkDescriptorSetLayout descriptorSetLayout,
        const PipelineConfig& config
    );
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = delete;
    Pipeline& operator=(Pipeline&&) = delete;

    VkPipeline handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_pipelineLayout; }

private:
    void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
    void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
    void defaultPipelineConfig(PipelineConfig& config);

    Context* m_context;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    VkRenderPass m_renderPass;
};