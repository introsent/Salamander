#include "main_scene_executor.h"
#include "image_transition_manager.h"
#include "loaders/gltf_loader.h"
#include "shared/shared_structs.h"

MainSceneExecutor::MainSceneExecutor(Resources resources) : m_resources(std::move(resources))
{
    m_currentDepthLayouts.fill(VK_IMAGE_LAYOUT_UNDEFINED);
}

void MainSceneExecutor::begin(VkCommandBuffer cmd, uint32_t imageIndex) {

    m_currentImageIndex = imageIndex;
    ImageTransitionManager::transitionColorAttachment(
        cmd,
        m_resources.swapChain->getCurrentImage(imageIndex),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    ImageTransitionManager::transitionDepthAttachment(
        cmd,
        m_resources.depthImages[*m_resources.currentFrame],
        m_currentDepthLayouts[*m_resources.currentFrame],
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );
    m_currentDepthLayouts[*m_resources.currentFrame] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

void MainSceneExecutor::execute(VkCommandBuffer cmd) {
    // Depth Pre-Pass
    VkRenderingAttachmentInfo depthPre = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = m_resources.depthImageViews[*m_resources.currentFrame],
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = m_resources.clearDepth
    };
    VkRenderingInfo preInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = {{0, 0}, m_resources.extent},
        .layerCount = 1,
        .colorAttachmentCount = 0,
        .pColorAttachments = nullptr,
        .pDepthAttachment = &depthPre
    };
    vkCmdBeginRendering(cmd, &preInfo);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.depthPipeline);
    setViewportAndScissor(cmd);
    bindBuffers(cmd, m_resources.depthPipelineLayout, m_resources.gBufferDescriptorSets);
    drawPrimitives(cmd, m_resources.depthPipelineLayout);
    vkCmdEndRendering(cmd);

    ImageTransitionManager::transitionDepthAttachment(
            cmd,
            m_resources.depthImages[*m_resources.currentFrame],
             m_currentDepthLayouts[*m_resources.currentFrame] ,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

    m_currentDepthLayouts[*m_resources.currentFrame] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;


    // Transition G-buffer images
    ImageTransitionManager::transitionColorAttachment(
        cmd, m_resources.gBufferAlbedoImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    ImageTransitionManager::transitionColorAttachment(
        cmd, m_resources.gBufferNormalImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    ImageTransitionManager::transitionColorAttachment(
        cmd, m_resources.gBufferParamsImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );





    // G-buffer Pass
    std::array<VkRenderingAttachmentInfo, 3> gBufferAttachments = {{
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = m_resources.gBufferAlbedoView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
        },
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = m_resources.gBufferNormalView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color = {0.0f, 0.0f, 0.0f, 0.0f}}
        },
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = m_resources.gBufferParamsView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color = {0.0f, 0.0f, 0.0f, 0.0f}}
        }
    }};
    VkRenderingAttachmentInfo depthAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = m_resources.depthImageViews[*m_resources.currentFrame],
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
    };
    VkRenderingInfo gBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = {{0, 0}, m_resources.extent},
        .layerCount = 1,
        .colorAttachmentCount = static_cast<uint32_t>(gBufferAttachments.size()),
        .pColorAttachments = gBufferAttachments.data(),
        .pDepthAttachment = &depthAttachment
    };
    vkCmdBeginRendering(cmd, &gBufferInfo);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.gBufferPipeline);
    setViewportAndScissor(cmd);
    bindBuffers(cmd, m_resources.gBufferPipelineLayout, m_resources.gBufferDescriptorSets);
    drawPrimitives(cmd, m_resources.gBufferPipelineLayout);
    vkCmdEndRendering(cmd);

    // Transition G-buffer to shader read-only
    ImageTransitionManager::transitionToShaderRead(
        cmd, m_resources.gBufferAlbedoImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    ImageTransitionManager::transitionToShaderRead(
        cmd, m_resources.gBufferNormalImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    ImageTransitionManager::transitionToShaderRead(
        cmd, m_resources.gBufferParamsImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    // Lighting Pass
    VkRenderingAttachmentInfo colorAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = m_resources.colorImageViews[m_currentImageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = m_resources.clearColor
    };
    VkRenderingInfo lightingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = {{0, 0}, m_resources.extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = nullptr
    };
    vkCmdBeginRendering(cmd, &lightingInfo);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.lightingPipeline);
    setViewportAndScissor(cmd);
    bindBuffers(cmd, m_resources.lightingPipelineLayout, m_resources.lightingDescriptorSets);
    vkCmdDraw(cmd, 3, 1, 0, 0); // Full-screen triangle
    vkCmdEndRendering(cmd);
}

void MainSceneExecutor::end(VkCommandBuffer cmd) {
    ImageTransitionManager::transitionToPresent(
        cmd,
        m_resources.swapChain->getCurrentImage(m_currentImageIndex),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );
}

void MainSceneExecutor::setViewportAndScissor(VkCommandBuffer cmd) const {
    VkViewport viewport = {0.0f, 0.0f, static_cast<float>(m_resources.extent.width), static_cast<float>(m_resources.extent.height), 0.0f, 1.0f};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor = {{0, 0}, m_resources.extent};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void MainSceneExecutor::bindBuffers(VkCommandBuffer cmd, VkPipelineLayout layout, const std::vector<VkDescriptorSet>& descriptorSets) const {
    vkCmdBindIndexBuffer(cmd, m_resources.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1,
        &descriptorSets[*m_resources.currentFrame], 0, nullptr
    );
}

void MainSceneExecutor::drawPrimitives(VkCommandBuffer cmd, VkPipelineLayout layout) const {
    for (const auto& primitive : m_resources.primitives) {
        PushConstants pc = {
            .vertexBufferAddress = m_resources.vertexBufferAddress,
            .baseColorTextureIndex = primitive.materialIndex,
            .metalRoughTextureIndex = primitive.metalRoughTextureIndex,
            .modelScale = globalScale
        };
        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pc);
        vkCmdDrawIndexed(cmd, primitive.indexCount, 1, primitive.indexOffset, 0, 0);
    }
}