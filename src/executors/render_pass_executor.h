//
// Created by ivans on 04/05/2025.
//

#ifndef SALAMANDER_RENDER_PASS_EXECUTOR_H
#define SALAMANDER_RENDER_PASS_EXECUTOR_H


#include <functional>
#include <vulkan/vulkan.h>

namespace Salamander::Executors {
    class RenderPassExecutor {
    public:
        virtual void begin(VkCommandBuffer cmd, uint32_t imageIndex) = 0;
        virtual void execute(VkCommandBuffer cmd) = 0;
        virtual void end(VkCommandBuffer cmd) = 0;
        virtual ~RenderPassExecutor() = default;

        virtual void setExtraUiCallback(const std::function<void(unsigned int)> & function) {};
    };
}


#endif //SALAMANDER_RENDER_PASS_EXECUTOR_H
