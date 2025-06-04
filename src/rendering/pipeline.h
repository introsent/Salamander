#pragma once
#include "pipeline_config.h"
#include "context.h"
#include "shared/shared_structs.h"


class Pipeline {
public:
    Pipeline(
        Context* context,
        VkDescriptorSetLayout descriptorSetLayout,
        const PipelineConfig& config,
        VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(PushConstants)
        }
    );

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = delete;
    Pipeline& operator=(Pipeline&&) = delete;

    VkPipeline handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_pipelineLayout; }

private:
    void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) const;
    void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout, VkPushConstantRange pushConstantRange);

    Context* m_context;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
};
