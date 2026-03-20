#include "imgui_pass_executor.h"
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <array>

namespace Salamander::Executors {
    ImGuiPassExecutor::ImGuiPassExecutor(Resources resources)
        : m_resources(std::move(resources)) {
    }

    void ImGuiPassExecutor::begin(VkCommandBuffer cmd, uint32_t imageIndex) {
        // Color attachment
        VkRenderingAttachmentInfo colorAttachment;

        // Depth attachment
        VkRenderingAttachmentInfo depthAttachment;

        const VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, m_resources.extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
            .pDepthAttachment = &depthAttachment
        };

        vkCmdBeginRendering(cmd, &renderingInfo);
    }


    void ImGuiPassExecutor::execute(VkCommandBuffer cmd) {
        setViewportAndScissor(cmd);
        // start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // replace ImGui::ShowDemoWindow(); with:
        ImGui::Begin("Stats");

        // show FPS
        ImGuiIO &io = ImGui::GetIO();
        ImGui::Text("FPS: %.1f", io.Framerate);

        // optionally show frame time
        ImGui::Text("Frame Time: %.3f ms/frame", 1000.0f / io.Framerate);

        ImGui::End();

        // render ImGui
        ImGui::Render();
        if (ImDrawData *drawData = ImGui::GetDrawData()) {
            ImGui_ImplVulkan_RenderDrawData(drawData, cmd, VK_NULL_HANDLE);
        }
    }

    void ImGuiPassExecutor::end(VkCommandBuffer cmd) {
        vkCmdEndRendering(cmd);
    }

    void ImGuiPassExecutor::setViewportAndScissor(VkCommandBuffer cmd) const {
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
}

