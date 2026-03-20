#pragma once
#include <array>
#include <string>
#include "context.h"
#include "push_constants.h"

namespace Salamander::Graphics::Pipeline {
    struct PipelineConfig {
        std::string vertShaderPath;
        std::string fragShaderPath;

        VkVertexInputBindingDescription bindingDescription;
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

        VkPipelineViewportStateCreateInfo viewportState{};

        VkPipelineRasterizationStateCreateInfo rasterizer{};

        VkPipelineMultisampleStateCreateInfo multisampling{};

        VkPipelineDepthStencilStateCreateInfo depthStencil{};

        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments; // Vector instead of single attachment
        VkPipelineColorBlendStateCreateInfo colorBlending{};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        VkPipelineRenderingCreateInfo rendering{};
    };


    class Pipeline {
    public:
        Pipeline(
            Core::Context *context,
            VkDescriptorSetLayout descriptorSetLayout,
            const PipelineConfig &config,
            VkPushConstantRange pushConstantRange = {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = sizeof(PushConstants)
            }
        );

        Pipeline(const Pipeline &) = delete;

        Pipeline &operator=(const Pipeline &) = delete;

        Pipeline(Pipeline &&) = delete;

        Pipeline &operator=(Pipeline &&) = delete;

        [[nodiscard]] VkPipeline handle() const { return m_pipeline; }
        [[nodiscard]] VkPipelineLayout layout() const { return m_pipelineLayout; }

    private:
        void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) const;

        void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout, VkPushConstantRange pushConstantRange);

        VkPipeline m_pipeline{};
        Core::Context *m_context;
        VkPipelineLayout m_pipelineLayout;
    };
}
