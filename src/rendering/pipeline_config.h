#pragma once
#include <string>
#include <array>
#include <vector>
#include <vulkan/vulkan.h>

// The configuration structure containing all required pipeline settings
struct PipelineConfig {
    // Shader file paths
    std::string vertShaderPath;
    std::string fragShaderPath;

    // Vertex input descriptions
    VkVertexInputBindingDescription bindingDescription;
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

    // Viewport state
    VkPipelineViewportStateCreateInfo viewportState{};

    // Rasterizer state
    VkPipelineRasterizationStateCreateInfo rasterizer{};

    // Multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling{};

    // Depth stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencil{};

    // Color blend configuration (CHANGED)
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;  // Vector instead of single attachment
    VkPipelineColorBlendStateCreateInfo colorBlending{};

    // Dynamic state
    VkPipelineDynamicStateCreateInfo dynamicState{};
};


/* Builder class for PipelineConfig.It allows you to customize your pipeline config
   while providing sensible defaults. */
class PipelineConfigBuilder {
public:
    PipelineConfigBuilder();

    // Setters with chaining (fluent interface)
    PipelineConfigBuilder& setVertShaderPath(const std::string& path);
    PipelineConfigBuilder& setFragShaderPath(const std::string& path);
    PipelineConfigBuilder& setVertexInput(const VkVertexInputBindingDescription& bindingDesc,
        const std::array<VkVertexInputAttributeDescription, 3>& attribDescs);
    PipelineConfigBuilder& setInputAssembly(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable);
    PipelineConfigBuilder& setViewportState(uint32_t viewportCount, uint32_t scissorCount);
    PipelineConfigBuilder& setRasterizer(VkPolygonMode polygonMode, float lineWidth,
        VkCullModeFlags cullMode, VkFrontFace frontFace);
    PipelineConfigBuilder& setMultisampling(VkSampleCountFlagBits samples);
    PipelineConfigBuilder& setDepthStencil(VkBool32 depthTestEnable, VkBool32 depthWriteEnable,
        VkCompareOp depthCompareOp);
    PipelineConfigBuilder& setColorBlendAttachment(VkBool32 blendEnable, VkColorComponentFlags colorWriteMask);
    PipelineConfigBuilder& setColorBlending(VkBool32 logicOpEnable, VkLogicOp logicOp, const std::array<float, 4>& blendConstants);
    PipelineConfigBuilder& setDynamicState(const std::vector<VkDynamicState>& states);

    // Final build method returns a PipelineConfig with the configured values
    PipelineConfig build() const;

private:
    PipelineConfig config;
};


// Factory class for creating pipeline configurations
class PipelineFactory {
public:
    // Returns a default pipeline configuration
    static PipelineConfig createDefaultPipelineConfig();
};

