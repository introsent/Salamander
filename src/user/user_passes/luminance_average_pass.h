//
// Created by ivans on 06/01/2026.
//

#ifndef SALAMANDER_LUMINANCE_AVERAGE_PASS_H
#define SALAMANDER_LUMINANCE_AVERAGE_PASS_H
#include "compute_pipeline.h"
#include "irender_pass.h"
#include "descriptors/descriptor_set_layout.h"
#include "user_descriptor_managers/main_descriptor_manager.h"

struct LuminanceAveragePushConstants {
    float minLogLum;
    float logLumRange;
    float deltaTime;
    float tau;
    uint32_t pixelCount;
};

class LuminanceAveragePass : public IRenderPass {
public:
    void initialize(const SharedResources& shared,
                   MainSceneGlobalData& globalData,
                   PassDependencies& dependencies) override;
    void cleanup() override;
    void recreateSwapChain() override;
    void execute(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t imageIndex) override;

private:
    void createPipeline();
    void createDescriptors();
    void createAttachments();

    // Resources
    const SharedResources* m_shared = nullptr;
    MainSceneGlobalData* m_globalData = nullptr;
    PassDependencies* m_dependencies = nullptr;

    std::unique_ptr<ComputePipeline> m_pipeline;
    std::unique_ptr<DescriptorSetLayout> m_descriptorLayout;
    std::unique_ptr<MainDescriptorManager> m_descriptorManager;

    std::array<Texture*, MAX_FRAMES_IN_FLIGHT> m_averageLuminanceTextures;
};



#endif //SALAMANDER_LUMINANCE_AVERAGE_PASS_H
