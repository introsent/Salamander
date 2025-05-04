// imgui_descriptor_manager.h
#pragma once
#include "../rendering/descriptors/descriptor_manager_base.h"

class ImGuiDescriptorManager : public DescriptorManagerBase {
public:
    explicit ImGuiDescriptorManager(VkDevice device);
    VkDescriptorPool getPool() const override { return m_pool; }

private:
    void createDescriptorPool();
};