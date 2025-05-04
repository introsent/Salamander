#include "imgui_wrapper.h"
#include "core/data_structures.h"

ImGuiWrapper::ImGuiWrapper(Context* context, Window* window, SwapChain* swapChain, DepthFormat* depthFormat, CommandManager* commandManager)
    : m_context(context), m_window(window), m_swapChain(swapChain), m_commandManager(commandManager)
{
    RenderPass::Config imguiPassConfig{
    .colorFormat = m_swapChain->format(),
    .depthFormat = depthFormat->handle(),
	.colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,  // Preserve existing content
    .colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
    .colorInitialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    .colorFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    .depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,  // Preserve existing depth
    .depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .depthInitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    .depthFinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
	m_renderPass = RenderPass::create(m_context, imguiPassConfig);

    createDescriptorPool();
    initImGui();
}

void ImGuiWrapper::createDescriptorPool() {
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(m_context->device(), &pool_info, nullptr, &m_imguiDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create ImGui descriptor pool");
    }

    VkDescriptorPool descriptorPoolCopy = m_imguiDescriptorPool;
    VkDevice deviceCopy = m_context->device();
    DeletionQueue::get().pushFunction("ImGuiDescriptorPool", [deviceCopy, descriptorPoolCopy]() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(deviceCopy, descriptorPoolCopy, nullptr);
        });
}

void ImGuiWrapper::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForVulkan(m_window->handle(), true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_context->instance();
    init_info.PhysicalDevice = m_context->physicalDevice();
    init_info.Device = m_context->device();
    init_info.QueueFamily = m_context->findQueueFamilies(m_context->physicalDevice()).graphicsFamily.value();
    init_info.Queue = m_context->graphicsQueue();
    init_info.DescriptorPool = m_imguiDescriptorPool;
    init_info.MinImageCount = m_swapChain->images().size();
    init_info.ImageCount = m_swapChain->images().size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.RenderPass = m_renderPass->handle();

    ImGui_ImplVulkan_Init(&init_info);


    // Upload fonts
    auto commandBuffer = m_commandManager->beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
    m_commandManager->endSingleTimeCommands(commandBuffer);
    ImGui_ImplVulkan_DestroyFontsTexture();

    ImGui::StyleColorsDark();
    m_initialized = true;
}

void ImGuiWrapper::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Example UI
    ImGui::ShowDemoWindow();
}

void ImGuiWrapper::endFrame(VkCommandBuffer commandBuffer,
    FramebufferManager* framebufferManager,
    uint32_t imageIndex)
{
    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData) {
        return; // Skip rendering if no draw data is available
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass->handle();
    renderPassInfo.framebuffer = framebufferManager->framebuffers()[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapChain->extent();

    VkClearValue clearValues[2] = {};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
}

void ImGuiWrapper::recreateSwapChain(SwapChain* newSwapChain) {
    m_swapChain = newSwapChain;
    ImGui_ImplVulkan_SetMinImageCount(m_swapChain->images().size());
}