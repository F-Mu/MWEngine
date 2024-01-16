#pragma once

#include "runtime/function/render/pass/pass_base.h"
#include "function/render/render_type.h"
namespace MW {
    class CubePass;

    class LutPass;

    struct PbrIblUBO{
        glm::mat4 projViewMatrix;
        glm::vec4 lights[maxLightsCount];
        glm::vec3 cameraPos;
    };
    struct PbrIblPassInitInfo : public RenderPassInitInfo {
        PassBase::Framebuffer *frameBuffer;

        explicit PbrIblPassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };
    class PbrIblPass : public PassBase {
    public:
        void preparePassData() override;
        void initialize(const RenderPassInitInfo *info) override;
        void draw() override;
        void updateAfterFramebufferRecreate();
    protected:
        void precompute(const RenderPassInitInfo *info);
        void createUniformBuffer();
        void createDescriptorSets();
        void createPipelines();
        void createLightingGlobalDescriptor();
        std::shared_ptr<CubePass>   prefilteredPass;
        std::shared_ptr<CubePass>   irradiancePass;
        std::shared_ptr<LutPass>    lutPass;
        Framebuffer* fatherFramebuffer;
        PbrIblUBO pbrIblUbo;
        VulkanBuffer pbrIblUboBuffer;
    };
}