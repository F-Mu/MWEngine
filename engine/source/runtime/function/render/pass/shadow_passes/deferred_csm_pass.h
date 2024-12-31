#pragma once

#include "depth_pass.h"
#include "cascade_shadow_map_pass.h"
#include <array>

namespace MW {
    class Skybox;
    class DeferredCSMPass : public CascadeShadowMapPass {
        void initialize(const RenderPassInitInfo *info) override;
//        void clean()override;
    protected:
        void createPipelines() override;
        void createDescriptorSets()override;
        void draw() override;
        std::shared_ptr<VulkanTextureCubeMap> skybox;
    };
}