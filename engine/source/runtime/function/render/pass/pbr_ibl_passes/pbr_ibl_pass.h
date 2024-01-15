#pragma once

#include "runtime/function/render/pass/pass_base.h"

namespace MW {
    class CubePass;

    class LutPass;

    class PbrIblPass : public PassBase {
    public:
        void preparePassData() override;

        void precompute();
    protected:
        void initialize(const RenderPassInitInfo *info) override;
        void draw() override;
        void createUniformBuffer();
        void createDescriptorSets();
        void createPipelines();
        std::shared_ptr<CubePass>   prefilteredPass;
        std::shared_ptr<CubePass>   irradiancePass;
        std::shared_ptr<LutPass>    lutPass;
    };
}