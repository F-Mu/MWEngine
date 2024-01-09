#pragma once

#include "function/render/pass/pass_base.h"
#include <array>

namespace MW {
    struct SSAOPassInitInfo : public RenderPassInitInfo {
        PassBase::Framebuffer* frameBuffer;

        explicit SSAOPassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };

    class SSAOPass : public PassBase {
    public:
        void draw() override;

        void initialize(const RenderPassInitInfo *info) override;

        void preparePassData() override;
    protected:
        void createUniformBuffer();

        virtual void createDescriptorSets();

        virtual void createPipelines();
    };
}