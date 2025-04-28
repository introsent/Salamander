#pragma once
#include "../core/context.h"

class DescriptorSetLayout final {
public:
    DescriptorSetLayout(const Context* context);
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout(DescriptorSetLayout&&) = delete;
    DescriptorSetLayout& operator=(DescriptorSetLayout&&) = delete;

    VkDescriptorSetLayout handle() const { return m_layout; }

private:
    const Context* m_context;
    VkDescriptorSetLayout m_layout;

    void createLayout();
};