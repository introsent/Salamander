#include "render_pass.h"
#include <array>
#include <stdexcept>

RenderPass::RenderPass(const Context* context, const Config& config)
    : m_context(context), m_config(config) {
    initialize();
}

RenderPass::~RenderPass() {
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_context->device(), m_renderPass, nullptr);
    }
}

std::unique_ptr<RenderPass> RenderPass::create(const Context* context, const Config& config) {
    return std::unique_ptr<RenderPass>(new RenderPass(context, config));
}

void RenderPass::initialize() {
    // Enable VK_KHR_create_renderpass2 if not using Vulkan 1.3+
    // (Vulkan 1.3+ has this as core)

    // Color attachment
    VkAttachmentDescription2 colorAttachment{
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .format = m_config.colorFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = m_config.colorLoadOp,
        .storeOp = m_config.colorStoreOp,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = m_config.colorInitialLayout,
        .finalLayout = m_config.colorFinalLayout
    };

    // Depth attachment
    VkAttachmentDescription2 depthAttachment{
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .format = m_config.depthFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = m_config.depthLoadOp,
        .storeOp = m_config.depthStoreOp,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = m_config.depthInitialLayout,
        .finalLayout = m_config.depthFinalLayout
    };

    // Attachment references
    VkAttachmentReference2 colorAttachmentRef{
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
    };

    VkAttachmentReference2 depthAttachmentRef{
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT
    };

    // Subpass
    VkSubpassDescription2 subpass{
        .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    // Dependency
    VkSubpassDependency2 dependency{
        .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
                       VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                         VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    // Create render pass
    std::array<VkAttachmentDescription2, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo2 renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    // Use vkCreateRenderPass2
    if (vkCreateRenderPass2(m_context->device(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}