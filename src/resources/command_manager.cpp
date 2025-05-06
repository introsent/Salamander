#include "command_manager.h"
#include "deletion_queue.h"
#include <array>
#include <stdexcept>

CommandManager::CommandManager(VkDevice device, uint32_t queueFamilyIndex, VkQueue graphicsQueue)
    : m_device(device), m_graphicsQueue(graphicsQueue) {
    m_commandPoolManager = CommandPoolManager::create(
        device,
        queueFamilyIndex,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    );
}

CommandManager::~CommandManager()
{
    VkDevice device = m_device;
    VkCommandPool pool = m_commandPoolManager->handle();

    DeletionQueue::get().pushFunction("CommandPool", [device, pool]() {
        vkDestroyCommandPool(device, pool, nullptr);
        });
}

VkCommandBuffer CommandManager::beginSingleTimeCommands() const {
    // Allocate a command buffer for one-time use
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPoolManager->handle();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate one-time command buffer!");
    }

    // Begin recording the command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording one-time command buffer!");
    }

    return commandBuffer;
}


void CommandManager::endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record one-time command buffer!");
    }

    VkCommandBufferSubmitInfo commandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = commandBuffer
    };

    VkSubmitInfo2 submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferSubmitInfo
    };

    if (vkQueueSubmit2(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit one-time command buffer!");
    }

    vkQueueWaitIdle(m_graphicsQueue);
    vkFreeCommandBuffers(m_device, m_commandPoolManager->handle(), 1, &commandBuffer);
}

std::unique_ptr<CommandBuffer> CommandManager::createCommandBuffer(VkCommandBufferLevel level) const {
    // Delegate command buffer allocation to the CommandPoolManager
    return m_commandPoolManager->allocateCommandBuffer(level);
}

void CommandManager::recordCommandBuffer(
    CommandBuffer& commandBuffer,
    VkRenderPass renderPass,
    VkFramebuffer framebuffer,
    VkExtent2D extent,
    VkPipeline pipeline,
    VkPipelineLayout pipelineLayout,
    VkBuffer vertexBuffer,
    VkBuffer indexBuffer,
    const std::vector<VkDescriptorSet>& descriptorSets,
    uint32_t currentFrame,
    const std::vector<uint32_t>& indices
)
{

    // Begin the render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer.handle(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the graphics pipeline
    vkCmdBindPipeline(commandBuffer.handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Set the viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

    // Set the scissor
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

    // Bind the vertex buffer
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer.handle(), 0, 1, &vertexBuffer, offsets);

    // Bind the index buffer
    vkCmdBindIndexBuffer(commandBuffer.handle(), indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Bind the descriptor sets
    vkCmdBindDescriptorSets(
        commandBuffer.handle(),
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1,
        &descriptorSets[currentFrame],
        0,
        nullptr
    );

    // Issue the draw command
    vkCmdDrawIndexed(commandBuffer.handle(), static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    // End the render pass
    vkCmdEndRenderPass(commandBuffer.handle());
}




