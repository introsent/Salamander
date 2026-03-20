#pragma once

#include <vulkan/vulkan.h>
#include <memory>

#include "textures/texture.h"

namespace Salamander::Renderer::Frame {
    // forward declarations
    class CommandBuffer;

    constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    constexpr uint32_t HISTOGRAM_BINS = 256;

    struct Frame {
        std::unique_ptr<CommandBuffer> commandBuffer;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
        Resources::Textures::Texture* depthTexture;
    };

}
