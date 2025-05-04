#include "imgui_pass_executor.h"
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <array>

ImGuiPassExecutor::ImGuiPassExecutor(RenderPass* renderPass, Resources resources)
    : m_renderPass(renderPass)
    , m_resources(std::move(resources)) 
{
}

void ImGuiPassExecutor::begin(VkCommandBuffer cmd, uint32_t imageIndex) 
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass->handle();
    renderPassInfo.framebuffer = m_resources.framebuffers[imageIndex];
    renderPassInfo.renderArea = {{0, 0}, m_resources.extent};

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void ImGuiPassExecutor::execute(VkCommandBuffer cmd) 
{
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Your ImGui drawing code here
    ImGui::ShowDemoWindow();
    // Add other ImGui windows/widgets as needed

    // Render ImGui
    ImGui::Render();
    if (ImDrawData* drawData = ImGui::GetDrawData()) {
        ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
    }
}

void ImGuiPassExecutor::end(VkCommandBuffer cmd) 
{
    vkCmdEndRenderPass(cmd);
}