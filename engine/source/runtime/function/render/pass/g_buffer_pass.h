#pragma once

#include "depth_pass.h"
#include "function/render/pass/pass_base.h"
#include <array>

namespace MW {
    struct GBufferPassInitInfo : public RenderPassInitInfo {
        std::shared_ptr<VkRenderPass> renderPass;
    };
    struct GBufferCameraProject {
        glm::mat4 projView;
    };
    enum G_BUFFER_TYPE : uint8_t {
        g_position = 0, g_normal, g_albedo, g_depth, g_count
    };

    class GBufferPass : public PassBase {
    public:
        void draw() override;

        void initialize(const RenderPassInitInfo *info) override;

        void preparePassData() override;

    private:
        void createUniformBuffer();

        void createDescriptorSets();

        void createPipelines();

        void createFramebuffers();

        void createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment);

        GBufferCameraProject gBufferCameraProject;
        VulkanBuffer cameraUboBuffer;
    };
}