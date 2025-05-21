#include "pipeline.h"
#include <fstream>
#include <stdexcept>

#include "deletion_queue.h"
#include "shared/shared_structs.h"

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

Pipeline::Pipeline(
    Context* context,
    VkDescriptorSetLayout descriptorSetLayout,
    const PipelineConfig& config,
    VkPushConstantRange pushConstantRange
) : m_context(context){
    createPipelineLayout(descriptorSetLayout, pushConstantRange);

    auto vertCode = readFile(config.vertShaderPath);
    std::vector<char> fragCode;
    bool hasFragmentShader = !config.fragShaderPath.empty();
    if (hasFragmentShader) {
        fragCode = readFile(config.fragShaderPath); // optional because of depth pre-pass
    }

    VkPipelineShaderStageCreateInfo shaderStages[2];
    uint32_t shaderStageCount = 1;

    VkShaderModule vertShaderModule;
    createShaderModule(vertCode, &vertShaderModule);

    shaderStages[0] = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };

    VkShaderModule fragShaderModule;
    if (hasFragmentShader) {
        createShaderModule(fragCode, &fragShaderModule);
        shaderStages[1] = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShaderModule,
            .pName = "main"
        };
        shaderStageCount++;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineRenderingCreateInfo renderingInfo = config.rendering;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStageCount;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &config.inputAssembly;
    pipelineInfo.pViewportState = &config.viewportState;
    pipelineInfo.pRasterizationState = &config.rasterizer;
    pipelineInfo.pMultisampleState = &config.multisampling;
    pipelineInfo.pDepthStencilState = &config.depthStencil;
    pipelineInfo.pColorBlendState = &config.colorBlending;
    pipelineInfo.pDynamicState = &config.dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.pNext = &renderingInfo;

    if (vkCreateGraphicsPipelines(
        m_context->device(),
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        nullptr,
        &m_pipeline
    ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    VkDevice   deviceCopy = m_context->device();
    VkPipeline pipelineCopy = m_pipeline;

    static int pipelineID = 0;
    DeletionQueue::get().pushFunction("Pipeline" + std::to_string(pipelineID++), [deviceCopy, pipelineCopy]() {
        vkDestroyPipeline(deviceCopy, pipelineCopy, nullptr);
        });

    vkDestroyShaderModule(m_context->device(), vertShaderModule, nullptr);
    if (hasFragmentShader) {
        vkDestroyShaderModule(m_context->device(), fragShaderModule, nullptr);
    }
}

void Pipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(m_context->device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }
}

void Pipeline::createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout, VkPushConstantRange pushConstantRange) {

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;


    if (vkCreatePipelineLayout(m_context->device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }

    VkDevice         deviceCopy = m_context->device();
    VkPipelineLayout layoutCopy = m_pipelineLayout;

    static int pipelineLayoutID = 0;
    DeletionQueue::get().pushFunction("PipelineLayout_" + std::to_string(pipelineLayoutID++), [deviceCopy, layoutCopy]() {
        vkDestroyPipelineLayout(deviceCopy, layoutCopy, nullptr);
        });
}