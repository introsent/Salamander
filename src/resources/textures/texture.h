//
// Created by ivans on 05/12/2025.
//

#ifndef SALAMANDER_TEXTURE_H
#define SALAMANDER_TEXTURE_H
#include "image.h"

namespace Salamander::Resources::Textures
{
    class Texture
    {
    public:
        explicit Texture(VkDevice device);
        ~Texture() = default;

        void create(std::unique_ptr<Image> img);
        void createImageView();

        void setSampler(VkSampler sampler) {
            m_sampler = sampler;
            m_ownsSampler = false;
        }
        void createSampler(); // Only creates if needed

        // for cube map
        void createCubeImageView();
        void createCubeSampler();

        // for depth
        void createDepthSampler(bool useComparison = false);


        [[nodiscard]] Image* getImage() const { return m_image.get(); };
        void setDebugName(const DebugMessenger* debug, const std::string& name);

        [[nodiscard]] VkDescriptorImageInfo getDescriptorInfo() const;
    private:
        VkDevice m_device;

        std::unique_ptr<Image> m_image;
        VkImageView m_imageView = VK_NULL_HANDLE;

        VkSampler m_sampler = VK_NULL_HANDLE;
        bool m_ownsSampler = false;


        uint32_t m_width  = 0;
        uint32_t m_height = 0;
        VkFormat m_format = VK_FORMAT_UNDEFINED;
        uint32_t m_mipLevels = 1;
    };
}

#endif //SALAMANDER_TEXTURE_H