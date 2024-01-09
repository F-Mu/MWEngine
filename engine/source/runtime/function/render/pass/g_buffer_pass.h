#pragma once

#include "depth_pass.h"
#include "function/render/pass/pass_base.h"
#include <array>

namespace MW {
    struct GBufferPassInitInfo : public RenderPassInitInfo {
        PassBase::Framebuffer* frameBuffer;

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

        void preparePassData() override;
        void updateAfterFramebufferRecreate();
    private:
        void createUniformBuffer();

        void createDescriptorSets();

        void createPipelines();

        void createGlobalDescriptorSets();

        GBufferCameraProject gBufferCameraProject;
        VulkanBuffer cameraUboBuffer;
        Framebuffer* fatherFramebuffer;
    };
}