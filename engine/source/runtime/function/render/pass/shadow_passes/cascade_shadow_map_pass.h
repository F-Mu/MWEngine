#pragma once

#include "depth_pass.h"
#include "runtime/function/render/pass/pass_base/pass_base.h"
#include <array>

namespace MW {
    constexpr uint32_t CASCADE_COUNT = 8;
    constexpr uint32_t MIN_CASCADE_COUNT = 4;
    struct debugPushConstant {
        uint32_t cascadeIndex;
    };
    struct Cascade {
        float splitDepth;
        glm::mat4 lightProjViewMat;
    };
    struct CSMCameraProject {
        glm::mat4 projection;
        glm::mat4 view;
    };
    //todo:改名
    struct UniformBufferObjectFS {
        float cascadeSplits[CASCADE_COUNT];
        glm::mat4 cascadeProjViewMat[CASCADE_COUNT];
        glm::mat4 projViewMatrix;
        glm::vec3 lightDir;
        float _padLightDir;
        glm::vec3 cameraPos;
        float _padCameraPos;
        int32_t colorCascades{0};
    };

    struct CSMPassInitInfo : public RenderPassInitInfo {
        PassBase::Framebuffer *frameBuffer;

        explicit CSMPassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };

    class CascadeShadowMapPass : public PassBase {
    public:
        void draw() override;

        void initialize(const RenderPassInitInfo *info) override;

        void clean()override;

        void preparePassData() override;

        void drawDepth();

        void updateAfterFramebufferRecreate();
    protected:
        void setCascadeSplits();

        void updateCascade();

//        void createRenderPass();

        void createCSMGlobalDescriptor();

        virtual void createUniformBuffer();

        virtual void createDescriptorSets();

        virtual void createPipelines();

        std::array<Cascade, CASCADE_COUNT> cascades;
        std::shared_ptr<DepthPass> depthPass;
        CSMCameraProject csmCameraProject;
        UniformBufferObjectFS shadowMapFSUbo;
        VulkanBuffer cameraUboBuffer;
        VulkanBuffer shadowMapFSBuffer;
        Framebuffer *fatherFramebuffer;
        float cascadeSplitLambda{0.95f};
        float cascadeSplits[CASCADE_COUNT];
    };
}