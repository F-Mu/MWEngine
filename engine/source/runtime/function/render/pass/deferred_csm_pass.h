#pragma once

#include "depth_pass.h"
#include "cascade_shadow_map_pass.h"
#include <array>

namespace MW {
    class DeferredCSMPass : public CascadeShadowMapPass {
    protected:
        void createPipelines() override;

        void draw() override;
    };
}