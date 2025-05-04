#pragma once
#include "core/context.h"
#include "core/window.h"
#include "core/swap_chain.h"
#include "core/image_views.h"
#include "rendering/render_pass.h"
#include "executors/main_scene_pass_executor.h"
#include "executors/imgui_pass_executor.h"
#include "resources/command_manager.h"
#include "resources/buffer_manager.h"
#include "resources/texture_manager.h"
#include "core/data_structures.h"
#include "rendering/depth_format.h"
#include <memory>
#include <vector>
#include "core/framebuffer_manager.h"
#include "rendering/descriptors/descriptor_set_layout.h"
#include "rendering/pipeline.h"
#include "resources/index_buffer.h"
#include "resources/uniform_buffer.h"
#include "resources/vertex_buffer.h"
#include "rendering/descriptors/descriptor_set_layout_builder.h"
#include "rendering/descriptors/descriptor_pool_builder.h"
#include "user/user_descriptor_managers/main_descriptor_manager.h"
#include "user/user_descriptor_managers/imgui_descriptor_manager.h"


class Renderer {
public:
    Renderer(Context* context,
        Window* window,
        VmaAllocator allocator,
        const std::string& modelPath,
        const std::string& texturePath);
    ~Renderer();


    void drawFrame();
    void recreateSwapChain();
    void markFramebufferResized();

private:
    struct Frame {
        std::unique_ptr<CommandBuffer> commandBuffer;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
    };

    void initializeVulkan();
    void initializeGraphicsPipeline();
    void initializeRenderingResources();

    void createUniformBuffers();
    void createCommandBuffers();
    void createSyncObjects();
    void loadModel(const std::string& modelPath);
    void cleanup();

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    Context* m_context;
    Window* m_window;
    VmaAllocator m_allocator;

    // Model data
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    // Core Vulkan resources
    std::unique_ptr<SwapChain> m_swapChain;
    std::unique_ptr<ImageViews> m_imageViews;
    std::unique_ptr<DepthFormat> m_depthFormat;

    // Render passes and executors
    std::unique_ptr<RenderPass> m_mainRenderPass;
    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<RenderPass> m_imguiRenderPass;

    std::vector<std::unique_ptr<RenderPassExecutor>> m_passExecutors;

    // Resource managers
    std::unique_ptr<CommandManager> m_commandManager;
    std::unique_ptr<BufferManager> m_bufferManager;
    std::unique_ptr<TextureManager> m_textureManager;
    std::unique_ptr<FramebufferManager> m_framebufferManager;


    std::unique_ptr<DescriptorSetLayout> m_mainSceneLayout;
    std::unique_ptr<MainDescriptorManager> m_mainDescriptorManager;
    std::unique_ptr<ImGuiDescriptorManager> m_imguiDescriptorManager;

    // Resources
    ManagedTexture m_depthImage;
    ManagedTexture m_textureImage;
    VertexBuffer m_vertexBuffer;
    IndexBuffer m_indexBuffer;
    std::vector<UniformBuffer> m_uniformBuffers;

    std::vector<Frame> m_frames;
    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;


    //Framebuffers
    FramebufferManager::FramebufferSet m_mainSceneFramebuffers {};
    FramebufferManager::FramebufferSet m_imguiFramebuffers {};
};
