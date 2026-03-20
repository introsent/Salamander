#include "luminance_histogram_pass.h"

#include <config.h>
#include "compute_pipeline.h"
#include "graphics/descriptors/descriptor_set_layout_builder.h"

namespace Salamander::Renderer::Passes {
    void LuminanceHistogramPass::initialize(const Salamander::Renderer::RenderContext &ctx,
                                                 Salamander::Scene::MainSceneData &globalData,
                                                 Salamander::Renderer::PassDependencies &dependencies) {
        m_ctx = &ctx;
        m_globalData = &globalData;
        m_dependencies = &dependencies;

        createAttachments();
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

    void LuminanceHistogramPass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t /*imageIndex*/) {
        vkCmdFillBuffer(cmd, m_histogramBuffers[frameIndex].buffer, 0,
                        Salamander::Renderer::HISTOGRAM_BINS * sizeof(uint32_t), 0);

        VkBufferMemoryBarrier2 barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
            .buffer = m_histogramBuffers[frameIndex].buffer,
            .offset = 0,
            .size = Salamander::Renderer::HISTOGRAM_BINS * sizeof(uint32_t)
        };

        VkDependencyInfo depInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &barrier
        };
        vkCmdPipelineBarrier2(cmd, &depInfo);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->handle());
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->layout(),
                                0, 1, &m_descriptorManager->getDescriptorSets()[frameIndex], 0, nullptr);

        const float logLumRange = 12.0f;
        LuminanceHistogramPushConstants pc{
            .minLogLum = -10.0f,
            .inverseLogLumRange = 1.0f / logLumRange
        };
        vkCmdPushConstants(cmd, m_pipeline->layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

        const VkExtent2D extent = m_ctx->swapChain().extent();
        vkCmdDispatch(cmd, (extent.width + 15) / 16, (extent.height + 15) / 16, 1);

        m_dependencies->hdrTextures[frameIndex]->getImage()->transitionLayoutEx(
            cmd,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_ACCESS_2_SHADER_READ_BIT
        );
    }

    void LuminanceHistogramPass::createPipeline() {
        ComputePipelineConfig config;
        config.computeShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/luminance_histogram_comp.spv";

        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = sizeof(LuminanceHistogramPushConstants)
        };

        m_pipeline = std::make_unique<ComputePipeline>(
            &m_ctx->context(),
            m_descriptorLayout->handle(),
            config,
            pushConstantRange
        );
    }

    void LuminanceHistogramPass::createDescriptors() {
        DescriptorSetLayoutBuilder layoutBuilder(m_ctx->context().device());
        m_descriptorLayout = layoutBuilder
                .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                .build();

        std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, Salamander::Renderer::MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Salamander::Renderer::MAX_FRAMES_IN_FLIGHT}
        };

        m_descriptorManager = std::make_unique<MainDescriptorManager>(
            m_ctx->context().device(),
            m_descriptorLayout->handle(),
            poolSizes,
            Salamander::Renderer::MAX_FRAMES_IN_FLIGHT
        );

        for (size_t i = 0; i < Salamander::Renderer::MAX_FRAMES_IN_FLIGHT; ++i) {
            VkDescriptorImageInfo hdrInfo{
                .sampler = m_dependencies->hdrTextures[i]->getDescriptorInfo().sampler,
                .imageView = m_dependencies->hdrTextures[i]->getDescriptorInfo().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            };
            VkDescriptorBufferInfo histogramInfo{
                .buffer = m_histogramBuffers[i].buffer,
                .offset = 0,
                .range = Salamander::Renderer::HISTOGRAM_BINS * sizeof(uint32_t)
            };

            std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates = {
                {
                    .binding = 0, .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .imageInfo = &hdrInfo, .descriptorCount = 1, .isImage = true
                },
                {
                    .binding = 1, .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .bufferInfo = &histogramInfo, .descriptorCount = 1, .isImage = false
                }
            };
            m_descriptorManager->updateDescriptorSet(i, updates);
        }
    }

    void LuminanceHistogramPass::createAttachments() {
        for (size_t i = 0; i < Salamander::Renderer::MAX_FRAMES_IN_FLIGHT; ++i) {
            m_histogramBuffers[i] = m_ctx->bufferManager().createBuffer(
                Salamander::Renderer::HISTOGRAM_BINS * sizeof(uint32_t),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            );
            m_dependencies->histogramBuffers[i] = m_histogramBuffers[i];
        }
    }
}
