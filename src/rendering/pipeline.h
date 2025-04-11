#pragma once
#include <array>
#include <string>
#include "pipeline_config.h"
#include "../core/context.h"


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

    Context* m_context;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    VkRenderPass m_renderPass;
};