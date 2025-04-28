// Renderer.h
#pragma once

#include <memory>
#include <string>
#include <vector>
#include "core/context.h"
#include "core/window.h"
#include "core/swap_chain.h"
#include "core/image_views.h"
#include "rendering/render_pass.h"
#include "rendering/pipeline.h"
#include "rendering/descriptor_set_layout.h"
#include "core/framebuffer_manager.h"
#include "rendering/descriptor_manager.h"
#include "resources/command_manager.h"
#include "resources/buffer_manager.h"
#include "resources/texture_manager.h"
#include "resources/index_buffer.h"
#include "resources/uniform_buffer.h"
#include "resources/vertex_buffer.h"
#include "core/data_structures.h" 
#include "rendering/depth_format.h"

class Renderer {
public:
    Renderer(Context* context,
        Window* window,
        VmaAllocator allocator,
        const std::string& modelPath,
        const std::string& texturePath);
    ~Renderer();

    void initVulkan();
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

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    Context* m_context;
    Window* m_window;
    VmaAllocator m_allocator;

    // Model data
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    // Vulkan resources as smart pointers
    std::unique_ptr<SwapChain>           m_swapChain;
    std::unique_ptr<ImageViews>          m_imageViews;
    std::unique_ptr<DepthFormat>         m_depthFormat;
    std::unique_ptr<RenderPass>          m_renderPass;
    std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout;
    std::unique_ptr<Pipeline>            m_pipeline;
    std::unique_ptr<FramebufferManager>  m_framebufferManager;
    std::unique_ptr<DescriptorManager>   m_descriptorManager;

    std::vector<Frame>   m_frames;
    uint32_t             m_currentFrame = 0;
    bool                 m_framebufferResized = false;

    std::unique_ptr<CommandManager> m_commandManager;
    std::unique_ptr<BufferManager>  m_bufferManager;
    std::unique_ptr<TextureManager> m_textureManager;

    ManagedTexture       m_depthImage;
    ManagedTexture       m_textureImage;
    VertexBuffer         m_vertexBuffer;
    IndexBuffer          m_indexBuffer;
    std::vector<UniformBuffer> m_uniformBuffers;

    void createUniformBuffers();
    void createCommandBuffers();
    void createSyncObjects();
    void loadModel(const std::string& modelPath);
    void cleanup();
};
