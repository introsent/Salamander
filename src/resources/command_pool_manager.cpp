#include "command_pool_manager.h"

CommandPoolManager::CommandPoolManager(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
    : m_device(device) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = flags;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

CommandPoolManager::~CommandPoolManager() {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}

std::unique_ptr<CommandBuffer> CommandPoolManager::allocateCommandBuffer(VkCommandBufferLevel level) {
    return std::make_unique<CommandBuffer>(m_device, m_commandPool, level);
}

void CommandPoolManager::recordCommandBuffer(CommandBuffer& commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extent) {
    commandBuffer.begin();

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

    // Additional commands can be added here...

    vkCmdEndRenderPass(commandBuffer.handle());
    commandBuffer.end();
}
