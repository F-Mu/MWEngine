#pragma once

#include "pass_base.h"
#include "function/render/render_model.h"
#include "ui_pass/VulkanUIOverlay.h"

namespace MW {
    class CascadeShadowMapPass;

    class ShadingPass;

    class GBufferPass;

    class SSAOPass;

    class PbrIblPass;

    class MainCameraPass : public PassBase {
    public:
        typedef std::function<void(UIOverlay* overlay)> UIFunc;

        void initialize(const RenderPassInitInfo *init_info) override;

        void preparePassData() override;

        void draw() override;

        virtual void updateAfterFramebufferRecreate();

        void drawUI(VkCommandBuffer commandBuffer);

        void updateOverlay(float delta_time);

        void clean();

        void processAfterPass();

        void registerOnKeyFunc(const UIFunc& func) { UIFunctions.push_back(func); }

    protected:
        VulkanBuffer cameraUniformBuffer;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::shared_ptr<SSAOPass> ssaoPass;
        std::shared_ptr<CascadeShadowMapPass> shadowMapPass;
        std::shared_ptr<ShadingPass> shadingPass;
        std::shared_ptr<PbrIblPass> lightingPass;
        std::shared_ptr<GBufferPass> gBufferPass;
        std::shared_ptr<UIOverlay> UIPass;
        std::vector<UIFunc> UIFunctions;
        Model scene;

        void loadModel();

        void createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment);

        void createAttachments();
#if USE_VRS
        void createShadingRateImage();
#endif
        virtual void OnUpdateUIOverlay(UIOverlay* overlay);

        virtual void createRenderPass();

        virtual void createDescriptorSets();

        virtual void createSwapchainFramebuffers();

        virtual void updateCamera();

        virtual void createUniformBuffer();
    };
} // namespace MW
