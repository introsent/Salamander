#include "descriptor_set_layout.h"
#include <array>
#include <stdexcept>

DescriptorSetLayout::DescriptorSetLayout(const Context* context) : m_context(context) {
    createLayout();
}

void DescriptorSetLayout::createLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layoutHandle;
    if (vkCreateDescriptorSetLayout(m_context->device(), &layoutInfo, nullptr, &layoutHandle) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

    VkDevice                 deviceCopy = m_context->device();
    VkDescriptorSetLayout    layoutCopy = layoutHandle;

    DeletionQueue::get().pushFunction("DescriptorSetLayout", [deviceCopy, layoutCopy]() {
        vkDestroyDescriptorSetLayout(deviceCopy, layoutCopy, nullptr);
        });
    m_layout = layoutHandle;
}