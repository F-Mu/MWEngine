#pragma once

#include "pass_base.h"

namespace MW {
    class RenderResourceBase;

    class MainCameraPass : public PassBase {
    public:
        void initialize(const RenderPassInitInfo &init_info) override;

        virtual void preparePassData() override;

        void draw() override;

        virtual void updateAfterFramebufferRecreate();

    protected:
        VulkanBuffer cameraUniformBuffer;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        virtual void createRenderPass();

        virtual void createPipelines();

        virtual void createDescriptorSets();

        virtual void createSwapchainFramebuffers();

        void updateCamera();
    };
} // namespace MW
