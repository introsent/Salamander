#pragma once

#include "core/context.h"
#include "core/window.h"
#include "core/swap_chain.h"
#include "rendering/render_pass.h"
#include "rendering/pipeline.h"
#include "resources/command_manager.h"
#include "resources/buffer_manager.h"
#include "resources/texture_manager.h"
#include "rendering/descriptor_manager.h"
#include "core/data_structures.h" // For Vertex definition

#include <memory>
#include <vector>

#include "core/framebuffer_manager.h"
#include "core/image_views.h"
#include "resources/index_buffer.h"
#include "resources/uniform_buffer.h"
#include "resources/vertex_buffer.h"

class Renderer {
public:
    Renderer(Context* context,
        Window* window,
        VmaAllocator allocator,
        const std::string& modelPath,
        const std::string& texturePath);
    ~Renderer();

    void initialize();
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

    constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    Context* m_context;
    Window* m_window;
    VmaAllocator m_allocator;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    std::unique_ptr<SwapChain> m_swapChain;
    std::unique_ptr<ImageViews> m_imageViews;
    std::unique_ptr<RenderPass> m_renderPass;
    std::unique_ptr<Pipeline> m_pipeline;
    std::unique_ptr<FramebufferManager> m_framebufferManager;
    std::unique_ptr<DescriptorManager> m_descriptorManager;

    std::vector<Frame> m_frames;
    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;

    CommandManager* m_commandManager;
    BufferManager* m_bufferManager;
    TextureManager* m_textureManager;

    ManagedTexture m_depthTexture;
    ManagedTexture m_modelTexture;

    VertexBuffer m_vertexBuffer;
    IndexBuffer m_indexBuffer;
    std::vector<UniformBuffer> m_uniformBuffers;

    void createSyncObjects();
    void createCommandBuffers();
    void createDepthResources();
    void updateUniformBuffer(uint32_t currentImage);
    void loadModel(const std::string& modelPath);
};
