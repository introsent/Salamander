//
// Created by ivans on 06/01/2026.
//

#include "luminance_average_pass.h"

#include <config.h>

#include "descriptors/descriptor_set_layout_builder.h"

void LuminanceAveragePass::initialize(const SharedResources &shared, MainSceneGlobalData &globalData,
                                      PassDependencies &dependencies) {
    m_shared = &shared;
    m_globalData = &globalData;
    m_dependencies = &dependencies;

    createAttachments();
    createDescriptors();
    createPipeline();
}

void LuminanceAveragePass::cleanup() {
    m_pipeline.reset();
    m_descriptorManager.reset();
    m_descriptorLayout.reset();
}

void LuminanceAveragePass::recreateSwapChain() {
}

void LuminanceAveragePass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) {
    m_averageLuminanceTextures[frameIndex]->getImage()->transitionLayoutEx(
        cmd,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_2_NONE,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_NONE,
        VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT
    );

    VkBufferMemoryBarrier2 bufferBarrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
        .buffer = m_dependencies->histogramBuffers[frameIndex].buffer,
        .offset = 0,
        .size = HISTOGRAM_BINS * sizeof(uint32_t)
    };

    VkDependencyInfo depInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &bufferBarrier
    };
    vkCmdPipelineBarrier2(cmd, &depInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->handle());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->layout(),
        0, 1, &m_descriptorManager->getDescriptorSets()[frameIndex], 0, nullptr);

    auto extent = m_shared->swapChain->extent();
    LuminanceAveragePushConstants pc = {
        .minLogLum = -10.0f,
        .logLumRange = 12.0f,
        .deltaTime = m_globalData->deltaTime,
        .tau = 1.1f,
        .pixelCount = extent.width * extent.height
    };

    vkCmdPushConstants(cmd, m_pipeline->layout(), VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(pc), &pc);

    // Single workgroup of 256 threads (one per histogram bin)
    vkCmdDispatch(cmd, 1, 1, 1);

    VkImageMemoryBarrier2 imageBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,  // keep as GENERAL since we read/write each frame
        .image = m_averageLuminanceTextures[frameIndex]->getImage()->handle(),
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
    };

    VkDependencyInfo depInfo2 = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier
    };
    vkCmdPipelineBarrier2(cmd, &depInfo2);

}

void LuminanceAveragePass::createPipeline() {
    ComputePipelineConfig config;
    config.computeShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/luminance_average_comp.spv";

    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(LuminanceAveragePushConstants)
    };

    m_pipeline = std::make_unique<ComputePipeline>(
        m_shared->context,
        m_descriptorLayout->handle(),
        config,
        pushConstantRange
    );

}

void LuminanceAveragePass::createDescriptors() {
    // descriptor layout
    DescriptorSetLayoutBuilder layoutBuilder(m_shared->context->device());
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // histogram
        .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)  // luminance texture
        .build();

    // descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_FRAMES_IN_FLIGHT}
    };

    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_descriptorLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    // update descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo histogramInfo = {
            .buffer = m_dependencies->histogramBuffers[i].buffer,
            .offset = 0,
            .range = HISTOGRAM_BINS * sizeof(uint32_t)
        };

        VkDescriptorImageInfo  luminanceImageInfo = {
            .sampler =  VK_NULL_HANDLE,
            .imageView = m_averageLuminanceTextures[i]->getDescriptorInfo().imageView,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        }
;
        std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates = {
            {
                .binding = 0,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .bufferInfo = &histogramInfo,
                .descriptorCount = 1,
                .isImage = false
            },
            {
                .binding = 1,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .imageInfo = &luminanceImageInfo,
                .descriptorCount = 1,
                .isImage = true
            }

        };
        m_descriptorManager->updateDescriptorSet(i, updates);
    }
}

void LuminanceAveragePass::createAttachments() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_averageLuminanceTextures[i] = &m_shared->textureManager->createTexture(
            1, 1,  // 1x1 pixel
            VK_FORMAT_R32_SFLOAT,  // single float
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VK_IMAGE_ASPECT_COLOR_BIT,
            false, true, "AverageLuminance"
        );
        m_dependencies->averageLuminanceTextures[i] = m_averageLuminanceTextures[i];
    }
}
