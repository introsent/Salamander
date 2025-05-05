#include "main_scene_pass_executor.h"
#include "render_pass.h"
#include <array>


MainScenePassExecutor::MainScenePassExecutor(RenderPass* renderPass, Resources resources)
        : m_renderPass(renderPass)
        , m_resources(std::move(resources)) {}

void MainScenePassExecutor::begin(VkCommandBuffer cmd, uint32_t imageIndex) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass->handle();
    renderPassInfo.framebuffer = m_resources.framebuffers[imageIndex];
    renderPassInfo.renderArea = {{0, 0}, m_resources.extent};

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void MainScenePassExecutor::execute(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipeline);
    setViewportAndScissor(cmd);
    bindBuffers(cmd);
    vkCmdDrawIndexed(cmd, m_resources.indices.size(), 1, 0, 0, 0);
}

void MainScenePassExecutor::end(VkCommandBuffer cmd) {
    vkCmdEndRenderPass(cmd);
}

void MainScenePassExecutor::setViewportAndScissor(VkCommandBuffer cmd) const {
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

void MainScenePassExecutor::bindBuffers(VkCommandBuffer cmd) const {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_resources.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_resources.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_resources.pipelineLayout, 0, 1, &m_resources.descriptorSets[0], 0, nullptr);
}








