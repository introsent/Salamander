//
// Created by ivans on 04/05/2025.
//

#ifndef SALAMANDER_MAIN_DESCRIPTOR_MANAGER_H
#define SALAMANDER_MAIN_DESCRIPTOR_MANAGER_H


#include "descriptors/descriptor_manager_base.h"
#include <vector>

#include "scene_data.h"

namespace Salamander::Graphics::Descriptors {
    class MainDescriptorManager : public DescriptorManagerBase {
    public:
        struct DescriptorUpdateInfo {
            uint32_t binding;
            VkDescriptorType type;

            union {
                const VkDescriptorBufferInfo *bufferInfo;
                const VkDescriptorImageInfo *imageInfo;
            };

            uint32_t descriptorCount;
            bool isImage;
        };

        MainDescriptorManager(
        VkDevice device,
        VkDescriptorSetLayout layout,

        const std::vector<VkDescriptorPoolSize> &poolSizes, uint32_t maxSets);

        void updateDescriptorSet(
            uint32_t setIndex,
            const std::vector<DescriptorUpdateInfo> &updateInfo
        ) const;

        [[nodiscard]] const std::vector<VkDescriptorSet> & getDescriptorSets() const
        {
            return m_descriptorSets;
        }
        [[nodiscard]] VkDescriptorPool getPool() const override
        {
            return m_pool;
        }

    private:
        VkDescriptorSetLayout m_layout;
        std::vector<VkDescriptorSet> m_descriptorSets;
    };
}


#endif //SALAMANDER_MAIN_DESCRIPTOR_MANAGER_H