//
// Created by ivans on 06/01/2026.
//

#ifndef SALAMANDER_COMPUTE_PIPELINE_H
#define SALAMANDER_COMPUTE_PIPELINE_H
#include <string>
#include "context.h"

namespace Salamander::Graphics::Pipeline {
    struct ComputePipelineConfig {
        std::string computeShaderPath;
    };

    class ComputePipeline {
    public:
        ComputePipeline(Context *context,
                        VkDescriptorSetLayout computePipelineLayout,
                        const ComputePipelineConfig &config,
                        VkPushConstantRange pushConstantRange);

        ComputePipeline(const ComputePipeline &) = delete;

        ComputePipeline &operator=(const ComputePipeline &) = delete;

        ComputePipeline(ComputePipeline &&) = delete;

        ComputePipeline &operator=(ComputePipeline &&) = delete;

        VkPipeline handle() const { return m_pipeline; }
        VkPipelineLayout layout() const { return m_pipelineLayout; }

    private:
        void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout, VkPushConstantRange pushConstantRange);

        void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) const;

        Context *m_context;
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
    };
}


#endif //SALAMANDER_COMPUTE_PIPELINE_H