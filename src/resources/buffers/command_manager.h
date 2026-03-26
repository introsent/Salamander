//
// Created by ivans on 13/04/2025.
//

#ifndef SALAMANDER_COMMAND_MANAGER_H
#define SALAMANDER_COMMAND_MANAGER_H


#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

#include "command_pool_manager.h"
#include "command_buffer.h"

namespace Salamander::Resources::Buffers
{
    class CommandManager {
    public:
        CommandManager(VkDevice device, uint32_t queueFamilyIndex, VkQueue graphicsQueue);
        ~CommandManager();
        CommandManager(const CommandManager &) = delete;
        CommandManager & operator=(const CommandManager &) = delete;
        CommandManager(CommandManager &&) = delete;
        CommandManager & operator=(CommandManager &&) = delete;

        // Begins a one-time command buffer
        [[nodiscard]] VkCommandBuffer beginSingleTimeCommands() const;

        // Ends and submits a one-time command buffer
        void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

        // Create a reusable CommandBuffer
        [[nodiscard]] std::unique_ptr<CommandBuffer> createCommandBuffer(
            VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

        // Record a command buffer
        static void recordCommandBuffer(
            const CommandBuffer &commandBuffer,
            VkRenderPass renderPass,
            VkFramebuffer framebuffer,
            VkExtent2D extent,
            VkPipeline pipeline,
            VkPipelineLayout pipelineLayout,
            VkBuffer vertexBuffer,
            VkBuffer indexBuffer,
            const std::vector<VkDescriptorSet> &descriptorSets,
            uint32_t currentFrame,
            const std::vector<uint32_t> &indices
        );


        // Accessors
        [[nodiscard]] VkDevice device() const { return m_device; }
        [[nodiscard]] VkQueue graphicsQueue() const { return m_graphicsQueue; }
        [[nodiscard]] VkCommandPool commandPool() const { return m_commandPoolManager->handle(); }

    private:
        VkDevice m_device;
        VkQueue m_graphicsQueue;
        std::shared_ptr<CommandPoolManager> m_commandPoolManager;
    };
}


#endif //SALAMANDER_COMMAND_MANAGER_H
