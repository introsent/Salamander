#include "pass_dependencies.h"
#include "resources/textures/texture.h"
#include "graphics/image_transition_manager.h"

namespace Salamander::Renderer::Passes {

    void PassDependencies::transitionDepth(VkCommandBuffer cmd, uint32_t frameIndex, VkImageLayout newLayout) {
        if (frameIndex >= depthTextures.size() || !depthTextures[frameIndex])
            return;

        Graphics::ImageTransitionManager::transition(
            cmd,
            depthTextures[frameIndex]->getImage(),
            depthTextures[frameIndex]->getCurrentLayout(),
            newLayout,
            VK_IMAGE_ASPECT_DEPTH_BIT
        );

        depthTextures[frameIndex]->setCurrentLayout(newLayout);
    }

}
