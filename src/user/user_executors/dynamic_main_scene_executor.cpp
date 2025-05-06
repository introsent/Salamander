#include "dynamic_main_scene_executor.h"
#include "image_transition_manager.h"

DynamicMainSceneExecutor::DynamicMainSceneExecutor(Resources resources)
    : m_resources(std::move(resources))
{
}

void DynamicMainSceneExecutor::begin(VkCommandBuffer cmd, uint32_t imageIndex) {
    m_currentImageIndex = imageIndex;

    // Get current swapchain extent
    VkExtent2D currentExtent = m_resources.swapChain->extent();
    if (currentExtent.width != m_resources.extent.width ||
        currentExtent.height != m_resources.extent.height) {
        // Update our stored extent
        m_resources.extent = currentExtent;
        }


    // Transition color attachment
    ImageTransitionManager::transitionColorAttachment(
        cmd,
        m_resources.swapChain->getCurrentImage(imageIndex)
    );

    m_colorAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_resources.colorImageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = m_resources.clearColor
    };

    m_depthAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_resources.depthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = m_resources.clearDepth
    };

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, m_resources.extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &m_colorAttachment,
        .pDepthAttachment = &m_depthAttachment
    };

    vkCmdBeginRendering(cmd, &renderingInfo);
}

void DynamicMainSceneExecutor::execute(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipeline);
    setViewportAndScissor(cmd);
    bindBuffers(cmd);
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_resources.indices.size()), 1, 0, 0, 0);
}

void DynamicMainSceneExecutor::end(VkCommandBuffer cmd) {
    vkCmdEndRendering(cmd);

    ImageTransitionManager::transitionToPresent(
        cmd,
        m_resources.swapChain->getCurrentImage(m_currentImageIndex)
    );
}

void DynamicMainSceneExecutor::updateResources(SwapChain *swapChain, VkImageView depthView) {
    m_resources.swapChain = swapChain;
    m_resources.depthImageView = depthView;
    m_resources.colorImageViews = swapChain->imagesViews();
    m_resources.extent = swapChain->extent();
}

void DynamicMainSceneExecutor::setViewportAndScissor(VkCommandBuffer cmd) const {
    VkViewport viewport{
        0.0f, 0.0f,
        static_cast<float>(m_resources.extent.width),
        static_cast<float>(m_resources.extent.height),
        0.0f, 1.0f
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, m_resources.extent};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void DynamicMainSceneExecutor::bindBuffers(VkCommandBuffer cmd) const {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_resources.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_resources.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_resources.pipelineLayout, 0, 1, &m_resources.descriptorSets[0], 0, nullptr);
}
