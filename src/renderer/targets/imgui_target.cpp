#include "imgui_target.h"
#include "executors/imgui_pass_executor.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "buffers/command_manager.h"

namespace Salamander::Renderer::Targets {

    void ImGuiTarget::initialize(const Frame::RenderContext &ctx) {
        m_ctx = &ctx;
        createDescriptors();
        createRenderingResources();
    }

    void ImGuiTarget::render(float /*deltaTime*/, VkCommandBuffer cmd, uint32_t imageIndex) {
        m_executor->begin(cmd, imageIndex);
        m_executor->execute(cmd);
        m_executor->end(cmd);
    }

    void ImGuiTarget::recreateSwapChain() {
        auto &frames = m_ctx->frames();
        std::array<VkImageView, Frame::MAX_FRAMES_IN_FLIGHT> depthViews;
        for (size_t i = 0; i < Frame::MAX_FRAMES_IN_FLIGHT; ++i)
            depthViews[i] = frames[i].depthTexture->getDescriptorInfo().imageView;

        Executors::ImGuiPassExecutor::Resources resources{
            .extent              = m_ctx->swapChain().extent(),
            .swapchainImageViews = m_ctx->swapChain().imagesViews(),
            .depthImageViews     = depthViews,
        };
        m_executor = std::make_unique<Executors::ImGuiPassExecutor>(std::move(resources));

        ImGuiIO &io   = ImGui::GetIO();
        io.DisplaySize = ImVec2(
            static_cast<float>(resources.extent.width),
            static_cast<float>(resources.extent.height)
        );
        ImGui_ImplVulkan_SetMinImageCount(
            static_cast<uint32_t>(m_ctx->swapChain().images().size())
        );
    }

    void ImGuiTarget::cleanup() {}

    // -------------------------------------------------------------------------

    void ImGuiTarget::createRenderingResources() {
        initializeImGui();

        auto &frames = m_ctx->frames();
        std::array<VkImageView, Frame::MAX_FRAMES_IN_FLIGHT> depthViews;
        for (size_t i = 0; i < Frame::MAX_FRAMES_IN_FLIGHT; ++i)
            depthViews[i] = frames[i].depthTexture->getDescriptorInfo().imageView;

        Executors::ImGuiPassExecutor::Resources resources{
            .extent              = m_ctx->swapChain().extent(),
            .swapchainImageViews = m_ctx->swapChain().imagesViews(),
            .depthImageViews     = depthViews,
        };
        m_executor = std::make_unique<Executors::ImGuiPassExecutor>(std::move(resources));
    }

    void ImGuiTarget::createDescriptors() {
        m_descriptorManager =
            std::make_unique<Graphics::Descriptors::ImGuiDescriptorManager>(
                m_ctx->context().device()
            );
    }

    void ImGuiTarget::initializeImGui() const {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui_ImplGlfw_InitForVulkan(m_ctx->window().handle(), true);

        const auto    &ctx      = m_ctx->context();
        const auto    &sc       = m_ctx->swapChain();
        const uint32_t imgCount = static_cast<uint32_t>(sc.images().size());

        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance        = ctx.instance();
        init_info.PhysicalDevice  = ctx.physicalDevice();
        init_info.Device          = ctx.device();
        init_info.QueueFamily     =
            ctx.findQueueFamilies(ctx.physicalDevice()).graphicsFamily.value();
        init_info.Queue           = ctx.graphicsQueue();
        init_info.PipelineCache   = VK_NULL_HANDLE;
        init_info.DescriptorPool  = m_descriptorManager->getPool();
        init_info.MinImageCount   = imgCount;
        init_info.ImageCount      = imgCount;
        init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
        init_info.UseDynamicRendering = true;

        VkFormat scFormat = sc.format();
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        renderingInfo.colorAttachmentCount    = 1;
        renderingInfo.pColorAttachmentFormats = &scFormat;
        renderingInfo.depthAttachmentFormat   = m_ctx->depthFormat();
        init_info.PipelineRenderingCreateInfo = renderingInfo;

        ImGui_ImplVulkan_Init(&init_info);

        auto cmd = m_ctx->commandManager().beginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture();
        m_ctx->commandManager().endSingleTimeCommands(cmd);
        ImGui_ImplVulkan_DestroyFontsTexture();
    }
}
