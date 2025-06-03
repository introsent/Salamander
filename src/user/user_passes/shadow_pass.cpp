#include "shadow_pass.h"

void ShadowPass::initialize(const RenderTarget::SharedResources &shared, MainSceneGlobalData &globalData,
    PassDependencies &dependencies) {

    m_globalData = &globalData;
    m_dependencies = &dependencies;
    m_shared = &shared;

    const glm::vec3 sceneCenter = (m_globalData->sceneAABB.min + m_globalData->sceneAABB.max) / 2.0f;
   // const glm::vec3 lightPosition = glm::vec3(sceneCenter.x, sceneCenter.y + 10.0f, sceneCenter.z);
}

void ShadowPass::cleanup() {
}

void ShadowPass::recreateSwapChain() {
}

void ShadowPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) {
}

void ShadowPass::createPipeline() {
}

void ShadowPass::createDescriptors() {
}

void ShadowPass::createLightMatrices() {
}

void ShadowPass::createUniformBuffers() {
}

void ShadowPass::createShadowMapTexture() {
}
