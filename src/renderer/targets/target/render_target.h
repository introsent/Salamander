//
// Created by ivans on 04/05/2025.
//

#ifndef SALAMANDER_RENDER_TARGET_H
#define SALAMANDER_RENDER_TARGET_H


#include <memory>
#include <vulkan/vulkan.h>

#include "render_pass_executor.h"
#include "renderer/frame/render_context.h"

namespace Salamander::Renderer::Targets {
    class RenderTarget {
    public:
        virtual ~RenderTarget() = default;

        virtual void initialize(const Frame::RenderContext &ctx) = 0;
        virtual void render(float deltaTime, VkCommandBuffer cmd, uint32_t imageIndex) = 0;
        virtual void recreateSwapChain() = 0;
        virtual void cleanup() = 0;
        virtual void updateUniformBuffers() const = 0;

    protected:
        const Frame::RenderContext *m_ctx = nullptr;
        std::unique_ptr<Executors::RenderPassExecutor> m_executor;
    };
}


#endif //SALAMANDER_RENDER_TARGET_H
