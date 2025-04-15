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

#include "core/framebuffer_manager.h"
#include "core/image_views.h"
#include "resources/index_buffer.h"
#include "resources/uniform_buffer.h"
#include "resources/vertex_buffer.h"

#include "rendering/descriptor_set_layout.h"
#include "rendering/depth_format.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <unordered_map>

class Renderer {
public:
    Renderer(Context* context,
        Window* window,
        VmaAllocator allocator,
        const std::string& modelPath,
        const std::string& texturePath);
    ~Renderer(); // Calls cleanup()

    // New version that matches the initVulkan order.
    void initVulkan();
    void drawFrame();
    void recreateSwapChain();

    void markFramebufferResized();

private:
    struct Frame {
        std::unique_ptr<CommandBuffer> commandBuffer; // Now raw pointer (ownership is manual)
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
    };

    // The number of frames; used in resource creation and sync objects.
    int MAX_FRAMES_IN_FLIGHT = 2;

    // Provided externally (ownership passed in)
    Context* m_context;
    Window* m_window;
    VmaAllocator m_allocator;

    // Model data.
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    // Resources that will be created/destroyed in our init/cleanup order.
    SwapChain* m_swapChain = nullptr;
    ImageViews* m_imageViews = nullptr;
    DepthFormat* m_depthFormat = nullptr;
    RenderPass* m_renderPass = nullptr;
    DescriptorSetLayout* m_descriptorSetLayout = nullptr;
    Pipeline* m_pipeline = nullptr;
    FramebufferManager* m_framebufferManager = nullptr;
    DescriptorManager* m_descriptorManager = nullptr;

    // Synchronization objects are held in a vector.
    std::vector<Frame>   m_frames;
    uint32_t             m_currentFrame = 0;
    bool                 m_framebufferResized = false;

    CommandManager* m_commandManager = nullptr;
    BufferManager* m_bufferManager = nullptr;
    TextureManager* m_textureManager = nullptr;

    // Textures: one for depth and one for the model/texture image.
    ManagedTexture       m_depthImage;
    ManagedTexture       m_textureImage;

    // Buffers.
    VertexBuffer         m_vertexBuffer;
    IndexBuffer          m_indexBuffer;
    std::vector<UniformBuffer> m_uniformBuffers;

    // Helpers for initialization.
    void createUniformBuffers();
    void createCommandBuffers();
    void createSyncObjects();
    void loadModel(const std::string& modelPath);

    // Cleanup used by the destructor.
    void cleanup();
};
