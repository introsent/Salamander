#include "../user_passes/depth_prepass.h"

#include "config.h"
#include "depth_format.h"
#include "image_transition_manager.h"
#include "pipeline.h"
#include "descriptors/descriptor_set_layout_builder.h"
#include "shared/scene_data.h"

#ifdef USE_TINYGLTF
    #include "loaders/gltf_loader.h"
#endif

void DepthPrepass::initialize(const RenderTarget::SharedResources& shared,
                              MainSceneGlobalData& globalData,
                              PassDependencies& dependencies) {
    m_shared = &shared;
    m_globalData = &globalData;
    m_dependencies = &dependencies;
    
    createDescriptors();
    createPipeline();
}

void DepthPrepass::cleanup() {
    m_pipeline.reset();
    m_descriptorManager.reset();
    m_descriptorLayout.reset();
}

void DepthPrepass::recreateSwapChain() {
    for (auto& layout : m_dependencies->depthLayouts) {
        layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

void DepthPrepass::execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) {

    // Transition per-frame depth image to initial layout
    ImageTransitionManager::transitionDepthAttachment(
        cmd,
        m_dependencies->perFrameDepthTextures[frameIndex]->image,
        m_dependencies->depthLayouts[*m_shared->currentFrame],
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    VkRenderingAttachmentInfo depthAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_dependencies->perFrameDepthTextures[frameIndex]->view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {.depthStencil = {1.0f, 0}}
    };

    VkRenderingInfo renderInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, m_shared->swapChain->extent()},
        .layerCount = 1,
        .colorAttachmentCount = 0,
        .pDepthAttachment = &depthAttachment
    };
    
    vkCmdBeginRendering(cmd, &renderInfo);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle());
    
    // Set dynamic viewport/scissor
    VkViewport viewport = {
        0.0f, 0.0f,
        static_cast<float>(m_shared->swapChain->extent().width),
        static_cast<float>(m_shared->swapChain->extent().height),
        0.0f, 1.0f
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    
    VkRect2D scissor = {{0, 0}, m_shared->swapChain->extent()};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    
    // Bind descriptor set
    vkCmdBindDescriptorSets(
        cmd, 
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipeline->layout(),
        0, 1,
        &m_descriptorManager->getDescriptorSets()[frameIndex],
        0, nullptr
    );
    
    // Bind index buffer
    vkCmdBindIndexBuffer(cmd, m_globalData->indexBuffer.handle(), 0, VK_INDEX_TYPE_UINT32);
    
    // Draw all primitives
    for (const auto& primitive : m_globalData->primitives) {
        PushConstants pc = {
            .vertexBufferAddress = m_globalData->vertexBufferAddress,
            .baseColorTextureIndex = primitive.materialIndex,
            .metalRoughTextureIndex = primitive.metalRoughTextureIndex,
            .normalTextureIndex = primitive.normalTextureIndex,
            .textureCount = static_cast<uint32_t>(m_globalData->modelTextures.size()),
            .modelScale = globalScale
        };
        vkCmdPushConstants(
            cmd, m_pipeline->layout(), 
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pc
        );
        vkCmdDrawIndexed(
            cmd, primitive.indexCount, 1, 
            primitive.indexOffset, 0, 0
        );
    }
    
    vkCmdEndRendering(cmd);

    // Update layout tracking
    m_dependencies->depthLayouts[frameIndex] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    ImageTransitionManager::transitionDepthAttachment(
           cmd,
           m_dependencies->perFrameDepthTextures[frameIndex]->image,
           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
           VK_IMAGE_LAYOUT_GENERAL
       );

    m_dependencies->depthLayouts[frameIndex] = VK_IMAGE_LAYOUT_GENERAL;

    // Transition the destination to TRANSFER_DST_OPTIMAL
    ImageTransitionManager::transitionDepthAttachment(
        cmd,
        m_dependencies->depthTextures[frameIndex]->image,
        VK_IMAGE_LAYOUT_UNDEFINED,  // First use in this frame
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    // Copy depth from framebuffer depth to our sampling depth texture
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.extent = {m_shared->swapChain->extent().width, m_shared->swapChain->extent().height, 1};

    vkCmdCopyImage(
        cmd,
        m_dependencies->perFrameDepthTextures[frameIndex]->image,
        VK_IMAGE_LAYOUT_GENERAL,  // GENERAL layout supports transfer source operations
        m_dependencies->depthTextures[frameIndex]->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion
    );

    // Transition the depth texture to shader read-only optimal for sampling
    ImageTransitionManager::transitionDepthAttachment(
        cmd,
        m_dependencies->depthTextures[frameIndex]->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    );

    ImageTransitionManager::transitionDepthAttachment(
       cmd,
       m_dependencies->perFrameDepthTextures[frameIndex]->image,
       VK_IMAGE_LAYOUT_GENERAL,
       VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
   );
    m_dependencies->depthLayouts[frameIndex] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

}

void DepthPrepass::createPipeline() {
    static constexpr std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.depthAttachmentFormat = m_shared->depthFormat;
    
    PipelineConfig config{};
    config.vertShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/depth_vert.spv";
    config.fragShaderPath = std::string(BUILD_RESOURCE_DIR) + "/shaders/depth_frag.spv";
    
    config.inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    
    config.viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };
    
    config.rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };
    
    config.multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };
    
    config.depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };
    
    config.colorBlendAttachments.clear(); // No color attachments
    config.colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 0,
        .pAttachments = nullptr
    };
    
    config.dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    
    config.rendering = renderingInfo;
    
    m_pipeline = std::make_unique<Pipeline>(
        m_shared->context,
        m_descriptorLayout->handle(),
        config
    );
}

void DepthPrepass::createDescriptors() {
    // Descriptor layout (matches original)
    DescriptorSetLayoutBuilder layoutBuilder(m_shared->context->device());
    m_descriptorLayout = layoutBuilder
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                   static_cast<uint32_t>(m_globalData->modelTextures.size()))
        .build();

    // Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         static_cast<uint32_t>(m_globalData->modelTextures.size() * MAX_FRAMES_IN_FLIGHT)}
    };

    m_descriptorManager = std::make_unique<MainDescriptorManager>(
        m_shared->context->device(),
        m_descriptorLayout->handle(),
        poolSizes,
        MAX_FRAMES_IN_FLIGHT
    );

    // Update descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        std::vector<MainDescriptorManager::DescriptorUpdateInfo> updates = {
            {
                .binding = 0,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .bufferInfo = &m_globalData->frameData[i].bufferInfo,
                .descriptorCount = 1,
                .isImage = false
            },
            {
                .binding = 1,
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .imageInfo = m_globalData->frameData[i].textureImageInfos.data(),
                .descriptorCount = static_cast<uint32_t>(m_globalData->modelTextures.size()),
                .isImage = true
            }
        };
        m_descriptorManager->updateDescriptorSet(i, updates);
    }
}