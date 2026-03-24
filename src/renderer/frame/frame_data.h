#pragma once

#include <vulkan/vulkan.h>
#include <memory>

namespace Salamander::Resources::Textures {
    class Texture;
}

namespace Salamander::Resources::Buffers {
    class CommandBuffer;
}



namespace Salamander::Renderer::Frame {

    constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    constexpr uint32_t HISTOGRAM_BINS = 256;

    struct Frame {
        std::unique_ptr<Resources::Buffers::CommandBuffer> commandBuffer;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
        Resources::Textures::Texture* depthTexture;
    };

}
