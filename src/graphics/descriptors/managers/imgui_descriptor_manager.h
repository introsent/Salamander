//
// Created by ivans on 04/05/2025.
//

#ifndef SALAMANDER_IMGUI_DESCRIPTOR_MANAGER_H
#define SALAMANDER_IMGUI_DESCRIPTOR_MANAGER_H


#include "descriptors/descriptor_manager_base.h"

namespace Salamander::Graphics::Descriptors{
    class ImGuiDescriptorManager final : public DescriptorManagerBase {
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


#endif //SALAMANDER_IMGUI_DESCRIPTOR_MANAGER_H
