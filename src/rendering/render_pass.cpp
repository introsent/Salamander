#include "render_pass.h"
#include <stdexcept>
#include <array>

RenderPass::RenderPass(const Context* context, VkFormat swapChainFormat, VkFormat depthFormat)
    : m_context(context) {
    createRenderPass(swapChainFormat, depthFormat);
}

void RenderPass::createRenderPass(VkFormat swapChainFormat, VkFormat depthFormat) {
    RenderPassBuilder builder(m_context, swapChainFormat, depthFormat);
    m_renderPass = builder.build();
}

// Builder implementation
RenderPassBuilder::RenderPassBuilder(const Context* context,
    VkFormat swapChainFormat,
    VkFormat depthFormat)
    : m_context(context),
    m_swapChainFormat(swapChainFormat),
    m_depthFormat(depthFormat) {
}

RenderPassBuilder::AttachmentSetups RenderPassBuilder::createBaseAttachments() {
    AttachmentSetups setups{};

    // Color attachment setup
    setups.color.format = m_swapChainFormat;
    setups.color.samples = VK_SAMPLE_COUNT_1_BIT;
    setups.color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    setups.color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    setups.color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    setups.color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    setups.color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    setups.color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment setup
    setups.depth.format = m_depthFormat;
    setups.depth.samples = VK_SAMPLE_COUNT_1_BIT;
    setups.depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    setups.depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    setups.depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    setups.depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    setups.depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    setups.depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Attachment references
    setups.colorRef.attachment = 0;
    setups.colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    setups.depthRef.attachment = 1;
    setups.depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass setup
    setups.subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    setups.subpass.colorAttachmentCount = 1;
    setups.subpass.pColorAttachments = &setups.colorRef;
    setups.subpass.pDepthStencilAttachment = &setups.depthRef;

    // Dependency setup
    setups.dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    setups.dependency.dstSubpass = 0;
    setups.dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    setups.dependency.srcAccessMask = 0;
    setups.dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    setups.dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    return setups;
}

VkRenderPass RenderPassBuilder::build() {
    auto setups = createBaseAttachments();

    std::array<VkAttachmentDescription, 2> attachments = {
        setups.color, setups.depth
    };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &setups.subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &setups.dependency;

    VkRenderPass renderPassHandle;
    if (vkCreateRenderPass(m_context->device(), &renderPassInfo, nullptr, &renderPassHandle) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }

    VkDevice deviceCopy = m_context->device();
    VkRenderPass renderPassCopy = renderPassHandle;

    DeletionQueue::get().pushFunction("RenderPass", [deviceCopy, renderPassCopy]() {
        vkDestroyRenderPass(deviceCopy, renderPassCopy, nullptr);
        });

    return renderPassHandle;
}
