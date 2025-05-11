#include "main_scene_executor.h"

#include <iostream>

#include "image_transition_manager.h"
#include "loaders/gltf_loader.h"
#include "shared/shared_structs.h"

MainSceneExecutor::MainSceneExecutor(Resources resources)
    : m_resources(std::move(resources))
{
}

void MainSceneExecutor::begin(VkCommandBuffer cmd, uint32_t imageIndex) {
    m_currentImageIndex = imageIndex;

    // Transition color attachment
    ImageTransitionManager::transitionColorAttachment(
        cmd,
        m_resources.swapChain->getCurrentImage(imageIndex),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
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

void MainSceneExecutor::execute(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipeline);
    setViewportAndScissor(cmd);
    bindBuffers(cmd);

    for (const auto& primitive : m_resources.primitives) {
        PushConstants pc = {
            .vertexBufferAddress = m_resources.vertexBufferAddress,
            .indexOffset = primitive.indexOffset,
            .materialIndex = primitive.materialIndex,
            .modelScale = globalScale
        };

        vkCmdPushConstants(
            cmd,
            m_resources.pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(PushConstants),
            &pc
        );

        vkCmdDrawIndexed(
            cmd,
            primitive.indexCount,
            1,
            primitive.indexOffset,
            0,
            0
        );
    }
}


void MainSceneExecutor::end(VkCommandBuffer cmd) {
    vkCmdEndRendering(cmd);

    ImageTransitionManager::transitionToPresent(
        cmd,
        m_resources.swapChain->getCurrentImage(m_currentImageIndex),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );
}


void MainSceneExecutor::setViewportAndScissor(VkCommandBuffer cmd) const {
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

void MainSceneExecutor::bindBuffers(VkCommandBuffer cmd) const {
    vkCmdBindIndexBuffer(cmd, m_resources.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_resources.pipelineLayout, 0, 1,
        &m_resources.descriptorSets[m_resources.currentFrame],  // Use currentFrame
        0, nullptr);

}
