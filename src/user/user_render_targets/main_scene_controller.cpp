#include "main_scene_controller.h"

#include <iostream>

#include "deletion_queue.h"
#include "depth_format.h"
#include "image_transition_manager.h"
#include "loaders/tinygltf_loader.h"

#ifdef USE_TINYGLTF
    #include "loaders/gltf_loader.h"
#endif

#ifdef USE_ASSIMP
    #include "loaders/assimp_loader.h"
#endif

void MainSceneController::initialize(const SharedResources& shared) {
    m_shared = &shared;
    loadModel(MODEL_PATH);
    createBuffers();

    constexpr uint32_t SHADOW_MAP_SIZE = 4096;
    m_dependencies.shadowMap = &m_shared->textureManager->createTexture(
        SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        false, true, "ShadowMapTexture"
    );

    m_cubeMapRenderer.initialize(m_shared->context,
                          m_shared->bufferManager,
                          m_shared->textureManager);

    m_shadowPass.initialize(shared, m_globalData, m_dependencies);
    createIBLResources();

    // Initialize passes in dependency order
    m_depthPrepass.initialize(shared, m_globalData, m_dependencies);
    m_gBufferPass.initialize(shared, m_globalData, m_dependencies);
    m_lightingPass.initialize(shared, m_globalData, m_dependencies);
    m_luminanceHistogramPass.initialize(shared, m_globalData, m_dependencies);
    m_luminanceAveragePass.initialize(shared, m_globalData, m_dependencies);
    m_toneMappingPass.initialize(shared, m_globalData, m_dependencies);


}

void MainSceneController::cleanup() {
    vkDeviceWaitIdle(m_shared->context->device());
    m_toneMappingPass.cleanup();
    m_luminanceAveragePass.cleanup();
    m_luminanceHistogramPass.cleanup();
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
    m_luminanceHistogramPass.recreateSwapChain();
    m_luminanceAveragePass.recreateSwapChain();
    m_toneMappingPass.recreateSwapChain();
}

void MainSceneController::render(float deltaTime, VkCommandBuffer cmd, uint32_t imageIndex) {
    m_globalData.deltaTime = deltaTime;
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
    m_luminanceHistogramPass.execute(cmd, *m_shared->currentFrame, imageIndex);
    m_luminanceAveragePass.execute(cmd, *m_shared->currentFrame, imageIndex);
    m_toneMappingPass.execute(cmd, *m_shared->currentFrame, imageIndex);


    //  transition INTO PRESENT_SRC_KHR
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

void MainSceneController::loadModel(const std::string& modelPath) {
#ifdef USE_TINYGLTF
    GLTFLoader::GLTFModel gltfModel;
    if (!TinyGLTFLoader::LoadFromFile(modelPath, gltfModel)) {
        throw std::runtime_error("Failed to load GLTF model");
    }
#endif
#ifdef USE_ASSIMP
    GLTFLoader::GLTFModel gltfModel;
    if (!AssimpLoader::LoadFromFile(modelPath, gltfModel)) {
        throw std::runtime_error("Failed to load GLTF model");
    }

#endif

    // clear previous data
    m_globalData.modelTextures.clear();
    m_globalData.materialTextures.clear();
    m_globalData.normalTextures.clear();

    if (gltfModel.vertices.empty()) {
        // empty model: set AABB to zero
        m_globalData.sceneAABB = { .min = glm::vec3(0.0f), .max = glm::vec3(0.0f) };
    } else {
        // initialize with extreme values
        auto minAABB = glm::vec3(std::numeric_limits<float>::max());
        auto maxAABB = glm::vec3(std::numeric_limits<float>::lowest());

        // find min/max across all vertices
        for (const auto& vertex : gltfModel.vertices) {
            minAABB = glm::min(minAABB, vertex.pos);
            maxAABB = glm::max(maxAABB, vertex.pos);
        }

        // store final AABB
        m_globalData.sceneAABB = { .min = minAABB, .max = maxAABB };
    }

    // create a default white texture for base color (index 0)
    unsigned char white[] = {255, 255, 255, 255};
    Texture& whiteTex = m_shared->textureManager->createTexture(white, 1, 1, 4, false, "default_white");
    m_globalData.modelTextures.push_back(&whiteTex);

    // separate texture maps for each type
    std::unordered_map<std::string, uint32_t> baseColorMap;
    std::unordered_map<std::string, uint32_t> normalMap;
    std::unordered_map<std::string, uint32_t> materialMap;
    std::vector<std::string> defaultMaterialKeys; // local deduplication

    // create SSBO for vertices
    m_globalData.vertexBuffer = SSBOBuffer(
        m_shared->bufferManager,
        m_shared->commandManager,
        m_shared->allocator,
        gltfModel.vertices.data(),
        sizeof(Vertex) * gltfModel.vertices.size()
    );
    m_globalData.vertexBufferAddress = m_globalData.vertexBuffer.getDeviceAddress(m_shared->context->device());

    // create an index buffer
    m_globalData.indexBuffer = IndexBuffer(
        m_shared->bufferManager,
        m_shared->commandManager,
        m_shared->allocator,
        gltfModel.indices
    );

    // process primitives
    m_globalData.primitives.clear();
    m_globalData.primitives.reserve(gltfModel.primitives.size());

    for (const auto& srcPrim : gltfModel.primitives) {
        uint32_t baseColorIndex = 0; // default to white texture
        uint32_t materialIndex = 0;
        uint32_t normalIndex = UINT32_MAX; // indicates no normal map

        if (srcPrim.materialIndex >= 0) {
            const auto& mat = gltfModel.materials[srcPrim.materialIndex];

            // load base color texture
            if (mat.baseColorTexture >= 0 && mat.baseColorTexture < gltfModel.textures.size()) {
                const auto& texInfo = gltfModel.textures[mat.baseColorTexture];
                std::string path = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + texInfo.uri;

                if (!baseColorMap.contains(path)) {
                    baseColorMap[path] = m_globalData.modelTextures.size();
                    Texture& baseTex = m_shared->textureManager->loadTexture(path, true, VK_FORMAT_R8G8B8A8_SRGB);
                    m_globalData.modelTextures.push_back(&baseTex);
                }
                baseColorIndex = baseColorMap[path];
            }

            // load normal texture
            if (mat.normalTexture >= 0 && mat.normalTexture < gltfModel.textures.size()) {
                const auto& texInfo = gltfModel.textures[mat.normalTexture];
                std::string path = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + texInfo.uri;

                if (!normalMap.contains(path)) {
                    normalMap[path] = m_globalData.normalTextures.size();
                    Texture& normalTex = m_shared->textureManager->loadTexture(path, true, VK_FORMAT_R8G8B8A8_UNORM);
                    m_globalData.normalTextures.push_back(&normalTex);
                }
                normalIndex = normalMap[path];
            }

            // load metallic-roughness texture
            if (mat.metallicRoughnessTexture >= 0 && mat.metallicRoughnessTexture < gltfModel.textures.size()) {
                const auto& texInfo = gltfModel.textures[mat.metallicRoughnessTexture];
                std::string path = std::string(SOURCE_RESOURCE_DIR) + "/models/sponza/" + texInfo.uri;

                if (!materialMap.contains(path)) {
                    materialMap[path] = m_globalData.materialTextures.size();
                    Texture& matTex = m_shared->textureManager->loadTexture(path, true, VK_FORMAT_R8G8B8A8_SRGB);
                    m_globalData.materialTextures.push_back(&matTex);
                }
                materialIndex = materialMap[path];
            } else {
                // create deduplicated default texture
                std::string key = "default_" +
                    std::to_string(mat.metallicFactor) + "_" +
                    std::to_string(mat.roughnessFactor);

                auto it = std::ranges::find(defaultMaterialKeys, key);
                if (it != defaultMaterialKeys.end()) {
                    materialIndex = std::distance(defaultMaterialKeys.begin(), it);
                } else {
                    unsigned char data[4] = {
                        0, // Unused
                        static_cast<unsigned char>(mat.roughnessFactor * 255),
                        static_cast<unsigned char>(mat.metallicFactor * 255),
                        255
                    };
                    std::string debugName = "default_material_" + std::to_string(mat.metallicFactor) + "_" + std::to_string(mat.roughnessFactor);
                    Texture& defaultMat = m_shared->textureManager->createTexture(data, 1, 1, 4, false, debugName);
                    m_globalData.materialTextures.push_back(&defaultMat);
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
    std::cout << "Loading finished" << std::endl;
}

void MainSceneController::createBuffers() {
    // create uniform buffers for each frame
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDeviceSize uboSize = sizeof(UniformBufferObject);
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

        // create an omni light buffer
        VkDeviceSize lightSize = sizeof(PointLightData);
        m_omniLightBuffer[i] = UniformBuffer(
            m_shared->bufferManager,
            m_shared->allocator,
            lightSize
        );
        PointLightData lightData{};
        lightData.pointLightPosition = glm::vec3(9.0f, 2.0f, -1.0f);
        lightData.pointLightIntensity = 100000.f;
        lightData.pointLightColor = glm::vec3(1.f, 0.f, 0.f);
        lightData.pointLightRadius = 10.f;
        m_omniLightBuffer[i].update(lightData);
        m_globalData.frameData[i].omniLightBufferInfo = {
            .buffer = m_omniLightBuffer[i].handle(),
            .offset = 0,
            .range = lightSize
        };

        // create a camera exposure buffer
        constexpr VkDeviceSize exposureSize = sizeof(CameraExposure);
        m_cameraExposureBuffer[i] = UniformBuffer(
            m_shared->bufferManager,
            m_shared->allocator,
            exposureSize
        );
        m_cameraExposureBuffer[i].update(camExpUBO);

        m_globalData.frameData[i].cameraExposureBufferInfo = {
            .buffer = m_cameraExposureBuffer[i].handle(),
            .offset = 0,
            .range = exposureSize
        };
    }

    // fill texture descriptor info for each frame
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // model textures (base color)
        m_globalData.frameData[i].textureImageInfos.clear();
        for (const auto& texPtr : m_globalData.modelTextures) {
            if (!texPtr) continue;
            VkDescriptorImageInfo desc = texPtr->getDescriptorInfo();
            m_globalData.frameData[i].textureImageInfos.push_back(desc);
        }

        // material textures (metallic-roughness)
        m_globalData.frameData[i].materialImageInfos.clear();
        for (const auto& texPtr : m_globalData.materialTextures) {
            if (!texPtr) continue;
            m_globalData.frameData[i].materialImageInfos.push_back(texPtr->getDescriptorInfo());
        }

        // normal textures
        m_globalData.frameData[i].normalImageInfos.clear();
        for (const auto& texPtr : m_globalData.normalTextures) {
            if (!texPtr) continue;
            m_globalData.frameData[i].normalImageInfos.push_back(texPtr->getDescriptorInfo());
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
    std::string debugName = "default_material_tex_" + std::to_string(m_globalData.materialTextures.size());
    Texture& texRef = m_shared->textureManager->createTexture(data, 1, 1, 4, false, debugName);
    m_globalData.materialTextures.push_back(&texRef);
    return static_cast<uint32_t>(m_globalData.materialTextures.size() - 1);
}

void MainSceneController::createIBLResources() {
    // load HDR
    m_hdrEquirect = &m_shared->textureManager->loadHDRTexture( std::string(SOURCE_RESOURCE_DIR) + "/textures/circus_arena.hdr");

    // create environment cube map
    m_envCubeMap = m_cubeMapRenderer.createCubeMap(1024, VK_FORMAT_R16G16B16A16_SFLOAT);

    // convert equirect to cube
    VkCommandBuffer cmd = m_shared->commandManager->beginSingleTimeCommands();
    m_cubeMapRenderer.renderEquirectToCube(cmd, m_hdrEquirect, m_envCubeMap);
    m_irradianceMap = m_cubeMapRenderer.createDiffuseIrradianceMap(cmd, m_envCubeMap, 128);
    m_shadowPass.execute(cmd, 0, 0);
    m_shared->commandManager->endSingleTimeCommands(cmd);

    // set in dependencies
    m_dependencies.equirectTexture = m_hdrEquirect;
    m_dependencies.cubeMap = m_envCubeMap.texture;
    m_dependencies.irradianceMap = m_irradianceMap.texture;
}
