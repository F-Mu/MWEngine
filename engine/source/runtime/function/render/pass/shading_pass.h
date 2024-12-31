#pragma once

#include "function/render/pass/pass_base.h"
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
namespace MW {
    struct ShadingPassInitInfo : public RenderPassInitInfo {
        PassBase::Framebuffer *frameBuffer;

        explicit ShadingPassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };

    struct ShadingParamsObject {
        float exposure{4.5f};
        float gamma{2.2f};
    };
    class ShadingPass : public PassBase {
    public:
        void draw() override;
        void initialize(const RenderPassInitInfo *info) override;
        void clean()override;
//        void preparePassData() override;
    protected:
        virtual void createPipelines();
        void createToneMappingUniformBuffer();
        void createToneMappingDescriptors();
        Framebuffer *fatherFrameBuffer;
        ShadingParamsObject shadingParamsUBO;
        VulkanBuffer shadingParamsUBOBuffer;
    };
}