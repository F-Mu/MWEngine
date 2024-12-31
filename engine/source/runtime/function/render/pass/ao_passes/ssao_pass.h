#pragma once

#include "function/render/pass/pass_base.h"
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "glm/glm.hpp"

namespace MW {
    constexpr uint32_t SSAO_KERNEL_SIZE = 64;
    constexpr float SSAO_RADIUS = 0.3f;
    constexpr uint32_t SSAO_NOISE_DIM = 4;
    struct SSAOUniformBufferObject {
        glm::mat4 projection;
    };

    struct SSAOPassInitInfo : public RenderPassInitInfo {
        PassBase::Framebuffer *frameBuffer;

        explicit SSAOPassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };

    class SSAOPass : public PassBase {
    public:
        void draw() override;

        void initialize(const RenderPassInitInfo *info) override;

        void clean();

        void preparePassData() override;

        void updateAfterFramebufferRecreate();
    protected:
        void createUniformBuffer();

        void createSSAOGlobalDescriptor();

        virtual void createDescriptorSets();

        virtual void createPipelines();

        VulkanBuffer SSAOKernelBuffer;
        VulkanTexture2D SSAONoise;
        VulkanBuffer cameraUboBuffer;
        SSAOUniformBufferObject SSAOUbo;
        Framebuffer* fatherFramebuffer;
    };
}