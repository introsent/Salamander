#include "imgui_target.h"
#include "user_executors/imgui_pass_executor.h"
#include "depth_format.h"
#include "descriptors/descriptor_set_layout_builder.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void ImGuiTarget::initialize(const SharedResources &shared) {
    m_shared = &shared;

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

    ImGuiPassExecutor::Resources resources{
        .extent =  m_shared->swapChain->extent(),
        .swapchainImageViews = m_shared->swapChain->imagesViews(),
        .depthImageView = m_shared->depthImageView
    };

    m_executor = std::make_unique<ImGuiPassExecutor>(std::move(resources));

    // Update ImGui display size
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(resources.extent.width),
                           static_cast<float>(resources.extent.height));

    ImGui_ImplVulkan_SetMinImageCount(m_shared->swapChain->images().size());
}

void ImGuiTarget::cleanup() {
}


void ImGuiTarget::createRenderingResources()
{
    initializeImGui();

    // ImGui pass executor
    ImGuiPassExecutor::Resources resources{
        .extent = m_shared->swapChain->extent(),
        .swapchainImageViews = m_shared->swapChain->imagesViews(),
        .depthImageView = m_shared->depthImageView
    };

    m_executor = std::make_unique<ImGuiPassExecutor>(std::move(resources));

}

void ImGuiTarget::createDescriptors() {
    m_descriptorManager = std::make_unique<ImGuiDescriptorManager>(m_shared->context->device());
}

void ImGuiTarget::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForVulkan(m_shared->window->handle(), true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_shared->context->instance();
    init_info.PhysicalDevice = m_shared->context->physicalDevice();
    init_info.Device = m_shared->context->device();
    init_info.QueueFamily = m_shared->context->findQueueFamilies(m_shared->context->physicalDevice()).graphicsFamily.value();
    init_info.Queue = m_shared->context->graphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_descriptorManager->getPool();
    init_info.MinImageCount = m_shared->swapChain->images().size();
    init_info.ImageCount =  m_shared->swapChain->images().size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.UseDynamicRendering = true;

    VkFormat attachmentFormat = m_shared->swapChain->format();
    VkPipelineRenderingCreateInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &attachmentFormat;
    renderingInfo.depthAttachmentFormat = m_shared->depthFormat;
    init_info.PipelineRenderingCreateInfo = renderingInfo;


    ImGui_ImplVulkan_Init(&init_info);

    // Upload ImGui fonts
    auto commandBuffer = m_shared->commandManager->beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
    m_shared->commandManager->endSingleTimeCommands(commandBuffer);
    ImGui_ImplVulkan_DestroyFontsTexture();
}
