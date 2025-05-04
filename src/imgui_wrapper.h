#pragma once

#include "core/context.h"
#include "core/framebuffer_manager.h"
#include "core/window.h"
#include "core/swap_chain.h"
#include "rendering/render_pass.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "rendering/depth_format.h"
#include "resources/command_manager.h"

class ImGuiWrapper {
public:
    ImGuiWrapper(Context* context, Window* window, SwapChain* swapChain, DepthFormat* depthFormat, CommandManager* commandManager);
    ~ImGuiWrapper();

    void beginFrame();
    void endFrame(VkCommandBuffer commandBuffer, FramebufferManager* framebufferManager, uint32_t imageIndex);
    void recreateSwapChain(SwapChain* newSwapChain);

private:
    void createDescriptorPool();
    void initImGui();

    Context* m_context;
    Window* m_window;
    SwapChain* m_swapChain;
	CommandManager* m_commandManager;

    std::unique_ptr<RenderPass>  m_renderPass;

    VkDescriptorPool m_imguiDescriptorPool;
    bool m_initialized = false;
};
