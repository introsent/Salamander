#include "pipeline_config.h"
#include "data_structures.h"

PipelineConfigBuilder::PipelineConfigBuilder() {
    // Set default shader paths
    config.vertShaderPath = "./shaders/shader_vert.spv";
    config.fragShaderPath = "./shaders/shader_frag.spv";

    // Set default vertex input info
    config.bindingDescription = Vertex::getBindingDescription();
    config.attributeDescriptions = Vertex::getAttributeDescriptions();

    // Input assembly defaults
    config.inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    config.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config.inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state defaults
    config.viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    config.viewportState.viewportCount = 1;
    config.viewportState.scissorCount = 1;

    // Rasterizer defaults
    config.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    config.rasterizer.depthClampEnable = VK_FALSE;
    config.rasterizer.rasterizerDiscardEnable = VK_FALSE;
    config.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    config.rasterizer.lineWidth = 1.0f;
    config.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    config.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    config.rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling defaults
    config.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    config.multisampling.sampleShadingEnable = VK_FALSE;
    config.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth stencil defaults
    config.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    config.depthStencil.depthTestEnable = VK_TRUE;
    config.depthStencil.depthWriteEnable = VK_TRUE;
    config.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    config.depthStencil.depthBoundsTestEnable = VK_FALSE;
    config.depthStencil.stencilTestEnable = VK_FALSE;

    // Color blend attachment defaults
    config.colorBlendAttachments.resize(1); // Use a vector for attachments
    VkPipelineColorBlendAttachmentState& colorBlendAttachment = config.colorBlendAttachments[0];
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // Color blend state defaults
    config.colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    config.colorBlending.logicOpEnable = VK_FALSE;
    config.colorBlending.logicOp = VK_LOGIC_OP_COPY;
    config.colorBlending.attachmentCount = static_cast<uint32_t>(config.colorBlendAttachments.size());
    config.colorBlending.pAttachments = config.colorBlendAttachments.data();
    config.colorBlending.blendConstants[0] = 0.0f;
    config.colorBlending.blendConstants[1] = 0.0f;
    config.colorBlending.blendConstants[2] = 0.0f;
    config.colorBlending.blendConstants[3] = 0.0f;

    // Dynamic state defaults
    static std::vector<VkDynamicState> defaultDynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    config.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    config.dynamicState.dynamicStateCount = static_cast<uint32_t>(defaultDynamicStates.size());
    config.dynamicState.pDynamicStates = defaultDynamicStates.data();
}


PipelineConfig PipelineConfigBuilder::build() const {
    PipelineConfig result = config;
    // Ensure the color blending attachments pointer points to the copied vector
    result.colorBlending.pAttachments = result.colorBlendAttachments.data();
    return result;
}

PipelineConfig PipelineFactory::createDefaultPipelineConfig() {
    PipelineConfigBuilder builder;
    return builder.build();
}