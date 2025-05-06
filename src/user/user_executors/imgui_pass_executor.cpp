#include "imgui_pass_executor.h"
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <array>

ImGuiPassExecutor::ImGuiPassExecutor(Resources resources)
    : m_resources(std::move(resources))
{
}

void ImGuiPassExecutor::begin(VkCommandBuffer cmd, uint32_t imageIndex)
{
    // Color attachment
    VkRenderingAttachmentInfo colorAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_resources.swapchainImageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}
    };

    // Depth attachment
    VkRenderingAttachmentInfo depthAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_resources.depthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE
    };

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, m_resources.extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = &depthAttachment
    };

    vkCmdBeginRendering(cmd, &renderingInfo);

    // Set viewport and scissor to match render area
    VkViewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_resources.extent.width),
        .height = static_cast<float>(m_resources.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{
        .offset = {0, 0},
        .extent = m_resources.extent
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void ImGuiPassExecutor::execute(VkCommandBuffer cmd)
{
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Your ImGui drawing code here
    ImGui::ShowDemoWindow();

    // Render ImGui
    ImGui::Render();
    if (ImDrawData* drawData = ImGui::GetDrawData()) {
        // Use updated render function for dynamic rendering
        ImGui_ImplVulkan_RenderDrawData(drawData, cmd, VK_NULL_HANDLE);
    }
}

void ImGuiPassExecutor::end(VkCommandBuffer cmd)
{
    vkCmdEndRendering(cmd);
}

