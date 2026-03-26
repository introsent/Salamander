//
// Created by ivans on 20/03/2026.
//

#ifndef SALAMANDER_PASS_DEPENDENCIES_H
#define SALAMANDER_PASS_DEPENDENCIES_H


#include <vulkan/vulkan.h>
#include <array>
#include "buffers/buffer_manager.h"
#include "renderer/frame/frame_data.h"

namespace Salamander::Renderer::Passes {
    /// PassDependencies manages interpass resource dependencies
    /// TODO: add to RenderGraph system.
    struct PassDependencies {
        // per-frame textures
        std::array<Resources::Textures::Texture*, Frame::MAX_FRAMES_IN_FLIGHT> albedoTextures{};
        std::array<Resources::Textures::Texture*, Frame::MAX_FRAMES_IN_FLIGHT> normalTextures{};
        std::array<Resources::Textures::Texture*, Frame::MAX_FRAMES_IN_FLIGHT> paramTextures{};
        std::array<Resources::Textures::Texture*, Frame::MAX_FRAMES_IN_FLIGHT> hdrTextures{};
        std::array<Resources::Buffers::ManagedBuffer, Frame::MAX_FRAMES_IN_FLIGHT> histogramBuffers{};
        std::array<Resources::Textures::Texture*, Frame::MAX_FRAMES_IN_FLIGHT> averageLuminanceTextures{};
        std::array<Resources::Textures::Texture*, Frame::MAX_FRAMES_IN_FLIGHT> depthTextures{};

        // static textures (IBL, shadow maps)
        Resources::Textures::Texture* equirectTexture{};
        Resources::Textures::Texture* cubeMap{};
        Resources::Textures::Texture* irradianceMap{};
        Resources::Textures::Texture* shadowMap{};
    };
}


#endif //SALAMANDER_PASS_DEPENDENCIES_H

