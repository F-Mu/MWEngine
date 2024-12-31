#pragma once

#include "runtime/function/render/pass/shadow_passes/depth_pass.h"
#include "function/render/pass/pass_base.h"
#include <array>

namespace MW {
    struct GBufferPassInitInfo : public RenderPassInitInfo {
        PassBase::Framebuffer *frameBuffer;

        explicit GBufferPassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };

    struct GBufferCameraProject {
        glm::mat4 projection;
        glm::mat4 view;
    };

    class GBufferPass : public PassBase {
    public:
        void draw() override;

        void initialize(const RenderPassInitInfo *info) override;

        void clean() override;

        void preparePassData() override;

        void updateAfterFramebufferRecreate();

    protected:
        virtual void createDescriptorSets();

        virtual void createPipelines();

        void createUniformBuffer();

        void createGlobalDescriptorSets();

        GBufferCameraProject gBufferCameraProject;
        VulkanBuffer cameraUboBuffer;
        Framebuffer *fatherFramebuffer;
    };
}