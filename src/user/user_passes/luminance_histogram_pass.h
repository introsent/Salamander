//
// Created by ivans on 06/01/2026.
//

#ifndef SALAMANDER_AVERAGE_LUMINANCE_CALCULATION_PASS_H
#define SALAMANDER_AVERAGE_LUMINANCE_CALCULATION_PASS_H
#include "compute_pipeline.h"
#include "irender_pass.h"
#include "pipeline.h"
#include "descriptors/descriptor_set_layout.h"
#include "user_descriptor_managers/main_descriptor_manager.h"

static constexpr uint32_t HISTOGRAM_BINS = 256;

struct LuminanceHistogramPushConstants {
    float minLogLum;
    float inverseLogLumRange;
};

class LuminanceHistogramPass : public IRenderPass {
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

    // Resources
    const SharedResources* m_shared = nullptr;
    MainSceneGlobalData* m_globalData = nullptr;
    PassDependencies* m_dependencies = nullptr;

    std::unique_ptr<ComputePipeline> m_pipeline;
    std::unique_ptr<DescriptorSetLayout> m_descriptorLayout;
    std::unique_ptr<MainDescriptorManager> m_descriptorManager;

    std::array<ManagedBuffer, MAX_FRAMES_IN_FLIGHT> m_histogramBuffers;
};


#endif //SALAMANDER_AVERAGE_LUMINANCE_CALCULATION_PASS_H