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
        ComputePipeline(Core::Context *context,
                        VkDescriptorSetLayout computePipelineLayout,
                        const ComputePipelineConfig &config,
                        VkPushConstantRange pushConstantRange);

        ComputePipeline(const ComputePipeline &) = delete;

        ComputePipeline &operator=(const ComputePipeline &) = delete;

        ComputePipeline(ComputePipeline &&) = delete;

        ComputePipeline &operator=(ComputePipeline &&) = delete;

        [[nodiscard]] VkPipeline handle() const { return m_pipeline; }
        [[nodiscard]] VkPipelineLayout layout() const { return m_pipelineLayout; }

    private:
        void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout, VkPushConstantRange pushConstantRange);

        void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) const;

        VkPipeline m_pipeline{};
        Core::Context *m_context;
        VkPipelineLayout m_pipelineLayout;
    };
}


#endif //SALAMANDER_COMPUTE_PIPELINE_H