#include "main_scene_target.h"

void MainSceneTarget::initialize(const SharedResources& shared) {
    m_controller.initialize(shared);
}

void MainSceneTarget::render(VkCommandBuffer cmd, uint32_t imageIndex) {
    m_controller.render(cmd, imageIndex);
}

void MainSceneTarget::recreateSwapChain() {
    m_controller.recreateSwapChain();
}

void MainSceneTarget::cleanup() {
    m_controller.cleanup();
}

void MainSceneTarget::updateUniformBuffers() const {
    m_controller.updateUniformBuffers();
}