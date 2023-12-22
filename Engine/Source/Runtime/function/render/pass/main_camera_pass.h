#pragma once

#include "pass_base.h"

namespace MW {
    class RenderResourceBase;

    class MainCameraPass : public PassBase {
    public:
        void initialize(const RenderPassInitInfo *init_info) override;

        void draw() override;

        void preparePassData(std::shared_ptr<RenderResource> render_resource) override;

    private:
        virtual void setupRenderPass();

        virtual void setupDescriptorSetLayout();

        virtual void setupPipelines();

        virtual void setupDescriptorSet();

        virtual void setupFramebufferDescriptorSet();

        virtual void setupSwapchainFramebuffers();

    private:
        std::vector<VkFramebuffer> swapChainFramebuffers;
    };
} // namespace MW
