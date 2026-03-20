#pragma once
#include "descriptors/descriptor_manager_base.h"

namespace Salamander::Graphics::Descriptors{
    class ImGuiDescriptorManager : public DescriptorManagerBase {
    public:
        explicit ImGuiDescriptorManager (VkDevice device);
        [[nodiscard]] VkDescriptorPool getPool() const override
        {
            return m_pool;
        }

    private:
        void createDescriptorPool();
    };
}
