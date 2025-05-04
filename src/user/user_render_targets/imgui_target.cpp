//
// Created by minaj on 5/4/2025.
//

#include "imgui_target.h"
#include "../user_executors/imgui_pass_executor.h"
#include "../rendering/depth_format.h"
#include "../rendering/descriptors/descriptor_set_layout_builder.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void ImGuiTarget::initialize(const SharedResources &shared) {
    m_shared = shared;

    createDescriptors();
    createRenderingResources();
}

void ImGuiTarget::render(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    m_executor->begin(commandBuffer, imageIndex);
    m_executor->execute(commandBuffer);
    m_executor->end(commandBuffer);
}

void ImGuiTarget::recreateSwapChain() {
    m_framebuffers = m_shared.framebufferManager->createFramebuffersForRenderPass(m_renderPass->handle());

    ImGuiPassExecutor::Resources imguiResources{
        .framebuffers = m_framebuffers.framebuffers,
        .extent = m_shared.swapChain->extent(),
        .imguiContext = ImGui::GetCurrentContext()
    };
    m_executor = std::make_unique<ImGuiPassExecutor>(m_renderPass.get(), imguiResources);

    ImGui_ImplVulkan_SetMinImageCount(m_shared.swapChain->images().size());
}

void ImGuiTarget::cleanup() {
}


void ImGuiTarget::createRenderingResources()
{
    RenderPass::Config imguiPassConfig{
        .colorFormat = m_shared.swapChain->format(),
        .depthFormat = DepthFormat(m_shared.context->physicalDevice()).handle(),
        .colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .colorInitialLayout =  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .colorFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .depthInitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .depthFinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    m_renderPass = RenderPass::create(m_shared.context, imguiPassConfig);

    initializeImGui();

    m_framebuffers = m_shared.framebufferManager->createFramebuffersForRenderPass(m_renderPass->handle());

    // ImGui pass executor
    ImGuiPassExecutor::Resources imguiResources{
        .framebuffers = m_framebuffers.framebuffers,
        .extent = m_shared.swapChain->extent(),
        .imguiContext = ImGui::GetCurrentContext()
    };
    m_executor = std::make_unique<ImGuiPassExecutor>(m_renderPass.get(), imguiResources);
}

void ImGuiTarget::createDescriptors() {
    m_descriptorManager = std::make_unique<ImGuiDescriptorManager>(m_shared.context->device());
}

void ImGuiTarget::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForVulkan(m_shared.window->handle(), true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_shared.context->instance();
    init_info.PhysicalDevice = m_shared.context->physicalDevice();
    init_info.Device = m_shared.context->device();
    init_info.QueueFamily = m_shared.context->findQueueFamilies(m_shared.context->physicalDevice()).graphicsFamily.value();
    init_info.Queue = m_shared.context->graphicsQueue();
    init_info.DescriptorPool = m_descriptorManager->getPool();
    init_info.MinImageCount = m_shared.swapChain->images().size();
    init_info.ImageCount =  m_shared.swapChain->images().size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.RenderPass = m_renderPass->handle();

    ImGui_ImplVulkan_Init(&init_info);

    // Upload ImGui fonts
    auto commandBuffer = m_shared.commandManager->beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
    m_shared.commandManager->endSingleTimeCommands(commandBuffer);
    ImGui_ImplVulkan_DestroyFontsTexture();
}
