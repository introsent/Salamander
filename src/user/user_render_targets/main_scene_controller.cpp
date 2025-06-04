#include "main_scene_controller.h"

#include "deletion_queue.h"
#include "loaders/gltf_loader.h"
#include "depth_format.h"
#include "image_transition_manager.h"

void MainSceneController::initialize(const RenderTarget::SharedResources& shared) {
    m_shared = &shared;
    
    createSamplers();
    loadModel(MODEL_PATH);
    createBuffers();

    // Initialize dependencies
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_dependencies.perFrameDepthTextures[i] = &(*shared.frames)[i].depthTexture;
    }

    m_cubeMapRenderer.initialize(m_shared->context,
                          m_shared->bufferManager,
                          m_shared->textureManager);

    m_shadowPass.initialize(shared, m_globalData, m_dependencies);
    createIBLResources();

    // Initialize passes in dependency order
    m_depthPrepass.initialize(shared, m_globalData, m_dependencies);
    m_gBufferPass.initialize(shared, m_globalData, m_dependencies);
    m_lightingPass.initialize(shared, m_globalData, m_dependencies);
    m_toneMappingPass.initialize(shared, m_globalData, m_dependencies);
}

void MainSceneController::cleanup() {
    vkDeviceWaitIdle(m_shared->context->device());
    m_toneMappingPass.cleanup();
    m_lightingPass.cleanup();
    m_gBufferPass.cleanup();
    m_depthPrepass.cleanup();
}

void MainSceneController::recreateSwapChain() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkWaitForFences(m_shared->context->device(), 1,
                        &(*m_shared->frames)[i].inFlightFence, VK_TRUE, UINT64_MAX);
    }
    m_depthPrepass.recreateSwapChain();
    m_gBufferPass.recreateSwapChain();
    m_lightingPass.recreateSwapChain();
    m_toneMappingPass.recreateSwapChain();
}

void MainSceneController::render(VkCommandBuffer cmd, uint32_t imageIndex) {

    updateUniformBuffers();

    // Transition swapchain images to initial layout
    ImageTransitionManager::transitionColorAttachment(
        cmd,
        m_shared->swapChain->getCurrentImage(imageIndex),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    // Execute passes in rendering order
    m_depthPrepass.execute(cmd, *m_shared->currentFrame, imageIndex);
    m_gBufferPass.execute(cmd, *m_shared->currentFrame, imageIndex);
    m_lightingPass.execute(cmd, *m_shared->currentFrame, imageIndex);
    m_toneMappingPass.execute(cmd, *m_shared->currentFrame, imageIndex);


    // ─── Finally transition INTO PRESENT_SRC_KHR ───
    ImageTransitionManager::transitionToPresent(
        cmd,
        m_shared->swapChain->getCurrentImage(imageIndex),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );

}

void MainSceneController::updateUniformBuffers() const {
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = m_shared->camera->GetViewMatrix();
    ubo.proj = m_shared->camera->GetProjectionMatrix(
        static_cast<float>(m_shared->swapChain->extent().width) /
        static_cast<float>(m_shared->swapChain->extent().height)
    );
    ubo.cameraPosition = m_shared->camera->Position;

    m_uniformBuffers[*m_shared->currentFrame].update(ubo);
}

void MainSceneController::createSamplers() {
    VkDevice deviceCopy   = m_shared->context->device();

    // G-buffer sampler
    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 8.0f,
        .compareEnable = VK_FALSE,
        .minLod = 0.0f,
        .maxLod = 8.0f,  // Match your mip levels
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };
    vkCreateSampler(m_shared->context->device(), &samplerInfo, nullptr, &m_globalData.gBufferSampler);

    VkSampler gbufferSamplerCopy = m_globalData.gBufferSampler;
    DeletionQueue::get().pushFunction("GbufferSampler_" + std::to_string(TextureManager::getSamplerIndex()), [deviceCopy, gbufferSamplerCopy]() {
        vkDestroySampler(deviceCopy, gbufferSamplerCopy, nullptr);
    });

    // Depth sampler
    VkSamplerCreateInfo depthSamplerInfo{};
    depthSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    depthSamplerInfo.magFilter = VK_FILTER_NEAREST;
    depthSamplerInfo.minFilter = VK_FILTER_NEAREST;
    depthSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    depthSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    depthSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(m_shared->context->device(), &depthSamplerInfo, nullptr, &m_globalData.depthSampler);

    VkSampler depthSamplerCopy = m_globalData.depthSampler;
    DeletionQueue::get().pushFunction("DepthSampler_" + std::to_string(TextureManager::getSamplerIndex()), [deviceCopy, depthSamplerCopy]() {
        vkDestroySampler(deviceCopy, depthSamplerCopy, nullptr);
    });


    // Shadow depth sampler
    VkSamplerCreateInfo shadowDepthSamplerInfo{};
    shadowDepthSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    shadowDepthSamplerInfo.magFilter = VK_FILTER_LINEAR;
    shadowDepthSamplerInfo.minFilter = VK_FILTER_LINEAR;
    shadowDepthSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shadowDepthSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shadowDepthSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shadowDepthSamplerInfo.compareEnable = VK_TRUE;
    shadowDepthSamplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    vkCreateSampler(m_shared->context->device(), &depthSamplerInfo, nullptr, &m_globalData.shadowDepthSampler);

    VkSampler shadowDepthSamplerCopy = m_globalData.shadowDepthSampler;
    DeletionQueue::get().pushFunction("ShadowDepthSampler_" + std::to_string(TextureManager::getSamplerIndex()), [deviceCopy,  shadowDepthSamplerCopy]() {
        vkDestroySampler(deviceCopy,  shadowDepthSamplerCopy, nullptr);
    });


    // HDR sampler
    VkSamplerCreateInfo hdrSamplerInfo{};
    hdrSamplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    hdrSamplerInfo.magFilter    = VK_FILTER_LINEAR;
    hdrSamplerInfo.minFilter    = VK_FILTER_LINEAR;
    hdrSamplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    hdrSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    hdrSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    hdrSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    hdrSamplerInfo.mipLodBias   = 0.0f;
    hdrSamplerInfo.minLod       = 0.0f;
    hdrSamplerInfo.maxLod       = 0.0f;
    hdrSamplerInfo.unnormalizedCoordinates = VK_FALSE;
    hdrSamplerInfo.anisotropyEnable        = VK_FALSE;
    hdrSamplerInfo.maxAnisotropy           = 1.0f;
    hdrSamplerInfo.compareEnable           = VK_FALSE;
    hdrSamplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    hdrSamplerInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    vkCreateSampler(m_shared->context->device(), &hdrSamplerInfo, nullptr, &m_globalData.hdrSampler);

    VkSampler samplerCopy = m_globalData.hdrSampler;
    DeletionQueue::get().pushFunction("ToneMappingSampler_" + std::to_string(TextureManager::getSamplerIndex()), [deviceCopy, samplerCopy]() {
        vkDestroySampler(deviceCopy, samplerCopy, nullptr);
    });
}

void MainSceneController::loadModel(const std::string& modelPath) {
    GLTFModel gltfModel;
    if (!GLTFLoader::LoadFromFile(modelPath, gltfModel)) {
        throw std::runtime_error("Failed to load GLTF model");
    }



    // Clear previous data
    m_globalData.modelTextures.clear();
    m_globalData.materialTextures.clear();
    m_globalData.normalTextures.clear();

    if (gltfModel.vertices.empty()) {
        // Empty model: set AABB to zero
        m_globalData.sceneAABB = { .min = glm::vec3(0.0f), .max = glm::vec3(0.0f) };
    } else {
        // Initialize with extreme values
        auto minAABB = glm::vec3(std::numeric_limits<float>::max());
        auto maxAABB = glm::vec3(std::numeric_limits<float>::lowest());

        // Find min/max across all vertices
        for (const auto& vertex : gltfModel.vertices) {
            minAABB = glm::min(minAABB, vertex.pos);
            maxAABB = glm::max(maxAABB, vertex.pos);
        }

        // Store final AABB
        m_globalData.sceneAABB = { .min = minAABB, .max = maxAABB };
    }

    // Create default white texture for base color (index 0)
    unsigned char white[] = {255, 255, 255, 255};
    m_globalData.modelTextures.push_back(
        m_shared->textureManager->createTexture(white, 1, 1, 4)
    );

    // Separate texture maps for each type
    std::unordered_map<std::string, uint32_t> baseColorMap;
    std::unordered_map<std::string, uint32_t> normalMap;
    std::unordered_map<std::string, uint32_t> materialMap;
    std::vector<std::string> defaultMaterialKeys; // Local deduplication

    // Create SSBO for vertices
    m_globalData.ssboBuffer = SSBOBuffer(
        m_shared->bufferManager,
        m_shared->commandManager,
        m_shared->allocator,
        gltfModel.vertices.data(),
        sizeof(Vertex) * gltfModel.vertices.size()
    );
    m_globalData.vertexBufferAddress = m_globalData.ssboBuffer.getDeviceAddress(m_shared->context->device());

    // Create index buffer
    m_globalData.indexBuffer = IndexBuffer(
        m_shared->bufferManager,
        m_shared->commandManager,
        m_shared->allocator,
        gltfModel.indices
    );

    // Process primitives
    m_globalData.primitives.clear();
    m_globalData.primitives.reserve(gltfModel.primitives.size());

    for (const auto& srcPrim : gltfModel.primitives) {
        uint32_t baseColorIndex = 0; // Default to white texture
        uint32_t materialIndex = 0;
        uint32_t normalIndex = UINT32_MAX; // Indicates no normal map

        if (srcPrim.materialIndex >= 0) {
            const auto& mat = gltfModel.materials[srcPrim.materialIndex];

            // Load base color texture
            if (mat.baseColorTexture >= 0 && mat.baseColorTexture < gltfModel.textures.size()) {
                const auto& texInfo = gltfModel.textures[mat.baseColorTexture];
                std::string path = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + texInfo.uri;

                if (!baseColorMap.count(path)) {
                    baseColorMap[path] = m_globalData.modelTextures.size();
                    m_globalData.modelTextures.push_back(
                        m_shared->textureManager->loadTexture(path)
                    );
                }
                baseColorIndex = baseColorMap[path];
            }

            // Load normal texture
            if (mat.normalTexture >= 0 && mat.normalTexture < gltfModel.textures.size()) {
                const auto& texInfo = gltfModel.textures[mat.normalTexture];
                std::string path = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + texInfo.uri;

                if (!normalMap.count(path)) {
                    normalMap[path] = m_globalData.normalTextures.size();
                    m_globalData.normalTextures.push_back(
                        m_shared->textureManager->loadTexture(path, VK_FORMAT_R8G8B8A8_UNORM)
                    );
                }
                normalIndex = normalMap[path];
            }

            // Load metallic-roughness texture
            if (mat.metallicRoughnessTexture >= 0 && mat.metallicRoughnessTexture < gltfModel.textures.size()) {
                const auto& texInfo = gltfModel.textures[mat.metallicRoughnessTexture];
                std::string path = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + texInfo.uri;

                if (!materialMap.count(path)) {
                    materialMap[path] = m_globalData.materialTextures.size();
                    m_globalData.materialTextures.push_back(
                        m_shared->textureManager->loadTexture(path, VK_FORMAT_R8G8B8A8_SRGB)
                    );
                }
                materialIndex = materialMap[path];
            } else {
                // Create deduplicated default texture
                std::string key = "default_" +
                    std::to_string(mat.metallicFactor) + "_" +
                    std::to_string(mat.roughnessFactor);

                auto it = std::find(defaultMaterialKeys.begin(), defaultMaterialKeys.end(), key);
                if (it != defaultMaterialKeys.end()) {
                    materialIndex = std::distance(defaultMaterialKeys.begin(), it);
                } else {
                    unsigned char data[4] = {
                        0, // Unused
                        static_cast<unsigned char>(mat.roughnessFactor * 255),
                        static_cast<unsigned char>(mat.metallicFactor * 255),
                        255
                    };
                    m_globalData.materialTextures.push_back(
                        m_shared->textureManager->createTexture(data, 1, 1, 4)
                    );
                    materialIndex = m_globalData.materialTextures.size() - 1;
                    defaultMaterialKeys.push_back(key);
                }
            }
        }

        m_globalData.primitives.push_back({
            .indexOffset = srcPrim.indexOffset,
            .indexCount = srcPrim.indexCount,
            .materialIndex = baseColorIndex,
            .metalRoughTextureIndex = materialIndex,
            .normalTextureIndex = normalIndex
        });
    }
}

void MainSceneController::createBuffers() {
    // Create uniform buffers for each frame
    VkDeviceSize uboSize = sizeof(UniformBufferObject);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_uniformBuffers[i] = UniformBuffer(
            m_shared->bufferManager,
            m_shared->allocator,
            uboSize
        );
        m_globalData.frameData[i].bufferInfo = {
            .buffer = m_uniformBuffers[i].handle(),
            .offset = 0,
            .range = uboSize
        };

        // Create omni light buffer
        VkDeviceSize lightSize = sizeof(PointLightData);
        m_omniLightBuffer[i] = UniformBuffer(
            m_shared->bufferManager,
            m_shared->allocator,
            lightSize
        );
        PointLightData lightData{};
        lightData.pointLightPosition = glm::vec3(8.0f, 1.0f, 0.0f);
        lightData.pointLightIntensity = 1000.f;
        lightData.pointLightColor = glm::vec3(0.f, 0.f, 1.f);
        lightData.pointLightRadius = 4.f;
        m_omniLightBuffer[i].update(lightData);
        m_globalData.frameData[i].omniLightBufferInfo = {
            .buffer = m_omniLightBuffer[i].handle(),
            .offset = 0,
            .range = lightSize
        };

        // Create camera exposure buffer
        VkDeviceSize exposureSize = sizeof(CameraExposure);
        m_cameraExposureBuffer[i] = UniformBuffer(
            m_shared->bufferManager,
            m_shared->allocator,
            exposureSize
        );
        CameraExposure exposureData{};
        m_cameraExposureBuffer[i].update(camExpUBO);

        m_globalData.frameData[i].cameraExposureBufferInfo = {
            .buffer = m_cameraExposureBuffer[i].handle(),
            .offset = 0,
            .range = exposureSize
        };
    }

    // Fill texture descriptor info for each frame
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Model textures (base color)
        m_globalData.frameData[i].textureImageInfos.clear();
        for (auto& tex : m_globalData.modelTextures) {
            m_globalData.frameData[i].textureImageInfos.push_back({
                .sampler = tex.sampler,
                .imageView = tex.view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }

        // Material textures (metallic-roughness)
        m_globalData.frameData[i].materialImageInfos.clear();
        for (auto& tex : m_globalData.materialTextures) {
            m_globalData.frameData[i].materialImageInfos.push_back({
                .sampler = tex.sampler,
                .imageView = tex.view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }

        // Normal textures
        m_globalData.frameData[i].normalImageInfos.clear();
        for (auto& tex : m_globalData.normalTextures) {
            m_globalData.frameData[i].normalImageInfos.push_back({
                .sampler = tex.sampler,
                .imageView = tex.view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }
    }
}

uint32_t MainSceneController::createDefaultMaterialTexture(float metallicFactor, float roughnessFactor) {
    unsigned char data[4] = {
        0,  // unused
        static_cast<unsigned char>(roughnessFactor * 255),
        static_cast<unsigned char>(metallicFactor * 255),
        255
    };
    ManagedTexture tex = m_shared->textureManager->createTexture(data, 1, 1, 4);
    m_globalData.materialTextures.push_back(tex);
    return static_cast<uint32_t>(m_globalData.materialTextures.size() - 1);
}

void MainSceneController::createIBLResources() {
    // Load HDR
    m_hdrEquirect = m_shared->textureManager->loadHDRTexture( std::string(SOURCE_RESOURCE_DIR) + "/textures/circus_arena.hdr");

    // Create environment cube map
    m_envCubeMap = m_cubeMapRenderer.createCubeMap(1024, VK_FORMAT_R16G16B16A16_SFLOAT);


    // Convert equirect to cube
    VkCommandBuffer cmd = m_shared->commandManager->beginSingleTimeCommands();
    m_cubeMapRenderer.renderEquirectToCube(cmd, m_hdrEquirect, m_envCubeMap);
    m_irradianceMap = m_cubeMapRenderer.createDiffuseIrradianceMap(cmd, m_envCubeMap, 128);
    m_shadowPass.execute(cmd, 0, 0);
    m_shared->commandManager->endSingleTimeCommands(cmd);

    // Set in dependencies
    m_dependencies.equirectTexture = &m_hdrEquirect;
    m_dependencies.cubeMap = &m_envCubeMap.texture;
    m_dependencies.cubeMap->view = m_envCubeMap.cubemapView;

    m_dependencies.irradianceMap = &m_irradianceMap.texture;
    m_dependencies.irradianceMap->view = m_irradianceMap.cubemapView;

}
