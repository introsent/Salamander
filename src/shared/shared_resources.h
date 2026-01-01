//
// Created by ivans on 31/12/2025.
//

#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

class Context;
class Window;
class SwapChain;
class CommandManager;
class BufferManager;
class TextureManager;
class Camera;
struct Frame;

struct SharedResources {
    Context* context;
    Window* window;
    SwapChain* swapChain;
    CommandManager* commandManager;
    BufferManager* bufferManager;
    TextureManager* textureManager;
    uint32_t* currentFrame;
    VmaAllocator allocator;
    VkImageView depthImageView;
    VkFormat depthFormat;
    Camera* camera;
    std::vector<Frame>* frames;
};