//
// Created by ivans on 06/01/2026.
//

#include "luminance_histogram_pass.h"

#include <config.h>

#include "compute_pipeline.h"
#include "descriptors/descriptor_set_layout_builder.h"

void LuminanceHistogramPass::initialize(const SharedResources &shared, MainSceneGlobalData &globalData,
                                                 PassDependencies &dependencies) {
    m_shared = &shared;
    m_globalData = &globalData;
    m_dependencies = &dependencies;

    createDescriptors();
    createPipeline();
}

void LuminanceHistogramPass::cleanup() {
    m_pipeline.reset();
    m_descriptorManager.reset();
    m_descriptorLayout.reset();
}

void LuminanceHistogramPass::recreateSwapChain() {
}

void LuminanceHistogramPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) {
    vkCmdFillBuffer(cmd, m_histogramBuffers[frameIndex].buffer, 0,
                    HISTOGRAM_BINS * sizeof(uint32_t), 0);


    VkBufferMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
        .buffer = m_histogramBuffers[frameIndex].buffer,
        .offset = 0,
        .size = HISTOGRAM_BINS * sizeof(uint32_t)
    };

    VkDependencyInfo depInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &barrier
    };
    vkCmdPipelineBarrier2(cmd, &depInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->handle());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->layout(),
        0, 1,
        &m_descriptorManager->getDescriptorSets()[frameIndex],
        0, nullptr);

    // push constants
    float logLumRange = 12.0f;  // covers luminance range from 2^-10 to 2^2
    LuminanceHistogramPushConstants pc = {
        .minLogLum = -10.0f,
        .inverseLogLumRange = 1.0f / logLumRange
    };
    vkCmdPushConstants(cmd, m_pipeline->layout(), VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(pc), &pc);

    auto extent = m_shared->swapChain->extent();
    vkCmdDispatch(cmd, (extent.width + 15) / 16, (extent.height + 15) / 16, 1);
}

void LuminanceHistogramPass::createPipeline() {
    ComputePipelineConfig config;
    config.computeShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/luminance_histogram_comp.spv";

    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(LuminanceHistogramPushConstants)
    };
    m_pipeline = std::make_unique<ComputePipeline>(
       m_shared->context,
       m_descriptorLayout->handle(),
       config,
       pushConstantRange
   );
}

void LuminanceHistogramPass::createDescriptors() {

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_histogramBuffers[i] = m_shared->bufferManager->createBuffer(
            HISTOGRAM_BINS * sizeof(uint32_t),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
        m_dependencies->histogramBuffers[i] = m_histogramBuffers[i];
    }
    // descriptor layout
    DescriptorSetLayoutBuilder layoutBuilder(m_shared->context->device());
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // hdr image
        .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)  // histogram
        .build();

    // descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_FRAMES_IN_FLIGHT}
    };

    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_descriptorLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    // update descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo hdrInfo = {
            .sampler = m_dependencies->hdrTextures[i]->getDescriptorInfo().sampler,
            .imageView = m_dependencies->hdrTextures[i]->getDescriptorInfo().imageView,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        };
        VkDescriptorBufferInfo histogramInfo = {
            .buffer = m_histogramBuffers[i].buffer,
            .offset = 0,
            .range = HISTOGRAM_BINS * sizeof(uint32_t)
        };

        std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates = {
            {
                .binding = 0,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .imageInfo = &hdrInfo,
                .descriptorCount = 1,
                .isImage = true
            },
            {
                .binding = 1,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .bufferInfo = &histogramInfo,
                .descriptorCount = 1,
                .isImage = false
            }
        };
        m_descriptorManager->updateDescriptorSet(i, updates);
    }
}
