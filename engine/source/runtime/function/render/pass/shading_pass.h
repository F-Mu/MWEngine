#pragma once

#include "function/render/pass/pass_base.h"
#include <array>

namespace MW {
    struct ShadingPassInitInfo : public RenderPassInitInfo {
        PassBase::Framebuffer* frameBuffer;

        explicit ShadingPassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };

    class ShadingPass : public PassBase {
    public:
        void draw() override;

        void initialize(const RenderPassInitInfo *info) override;

    protected:
        virtual void createPipelines();
        Framebuffer* fatherFrameBuffer;
    };
}