#pragma once

#include "pass_base.h"
#include "function/render/render_model.h"

namespace MW {
    class CascadeShadowMapPass;
    class ShadingPass;
    class GBufferPass;

    class MainCameraPass : public PassBase {
    public:
        void initialize(const RenderPassInitInfo *init_info) override;

        void preparePassData() override;

        void draw() override;

        virtual void updateAfterFramebufferRecreate();

    protected:
        VulkanBuffer cameraUniformBuffer;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::shared_ptr<CascadeShadowMapPass> shadowMapPass;
        std::shared_ptr<ShadingPass> shadingPass;
        std::shared_ptr<GBufferPass> gBufferPass;
        Model scene;

        void loadModel();

        void createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment);

        void createAttachments();

        virtual void createRenderPass();

        virtual void createPipelines();

        virtual void createDescriptorSets();

        virtual void createSwapchainFramebuffers();

        virtual void updateCamera();

        virtual void createUniformBuffer();
    };
} // namespace MW
