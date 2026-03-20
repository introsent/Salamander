//
// Created by ivans on 06/01/2026.
//

#include "compute_pipeline.h"

#include <fstream>

#include "deletion_queue.h"

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}


namespace Salamander::Graphics::Pipeline {
    ComputePipeline::ComputePipeline(Context *context,
                                          VkDescriptorSetLayout computePipelineLayout,
                                          const ComputePipelineConfig &config,
                                          VkPushConstantRange pushConstantRange) : m_context(context),
        m_pipelineLayout(nullptr) {
        createPipelineLayout(computePipelineLayout, pushConstantRange);

        auto computeShaderCode = readFile(config.computeShaderPath);

        VkShaderModule computeShaderModule;
        createShaderModule(computeShaderCode, &computeShaderModule);
        VkPipelineShaderStageCreateInfo shaderStage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = computeShaderModule,
            .pName = "main"
        };

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = shaderStage;
        pipelineInfo.layout = m_pipelineLayout;

        if (vkCreateComputePipelines(
                m_context->device(),
                VK_NULL_HANDLE,
                1,
                &pipelineInfo,
                nullptr,
                &m_pipeline
            ) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline");
        }

        VkDevice deviceCopy = m_context->device();
        VkPipeline pipelineCopy = m_pipeline;

        static int pipelineID = 0;
        DeletionQueue::get().pushFunction("ComputePipeline" + std::to_string(pipelineID++),
                                          [deviceCopy, pipelineCopy]() {
                                              vkDestroyPipeline(deviceCopy, pipelineCopy, nullptr);
                                          });

        vkDestroyShaderModule(m_context->device(), computeShaderModule, nullptr);
    }

    void ComputePipeline::createPipelineLayout(VkDescriptorSetLayout computePipelineLayout,
                                               VkPushConstantRange pushConstantRange) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &computePipelineLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;


        if (vkCreatePipelineLayout(m_context->device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline layout");
        }

        VkDevice deviceCopy = m_context->device();
        VkPipelineLayout layoutCopy = m_pipelineLayout;

        static int pipelineLayoutID = 0;
        DeletionQueue::get().pushFunction("ComputePipelineLayout_" + std::to_string(pipelineLayoutID++),
                                          [deviceCopy, layoutCopy]() {
                                              vkDestroyPipelineLayout(deviceCopy, layoutCopy, nullptr);
                                          });
    }

    void ComputePipeline::createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) const {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        if (vkCreateShaderModule(m_context->device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute shader module");
        }
    }
}
