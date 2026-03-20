#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include "buffers/buffer_manager.h"
#include "renderer/frame/frame_data.h"

class Texture;
namespace Salamander::Renderer::Passes {
    /// PassDependencies manages interpass resource dependencies
    /// TODO: add to RenderGraph system.
    struct PassDependencies {
        // per-frame textures
        std::array<Texture*, MAX_FRAMES_IN_FLIGHT> albedoTextures{};
        std::array<Texture*, MAX_FRAMES_IN_FLIGHT> normalTextures{};
        std::array<Texture*, MAX_FRAMES_IN_FLIGHT> paramTextures{};
        std::array<Texture*, MAX_FRAMES_IN_FLIGHT> hdrTextures{};
        std::array<ManagedBuffer, MAX_FRAMES_IN_FLIGHT> histogramBuffers{};
        std::array<Texture*, MAX_FRAMES_IN_FLIGHT> averageLuminanceTextures{};

        // static textures (IBL, shadow maps)
        Texture* equirectTexture{};
        Texture* cubeMap{};
        Texture* irradianceMap{};
        Texture* shadowMap{};

        // helper to transition depth attachment layouts
        void transitionDepth(VkCommandBuffer cmd, uint32_t frameIndex,
                             VkImageLayout newLayout);
    };
}

