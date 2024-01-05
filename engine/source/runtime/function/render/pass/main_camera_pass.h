#pragma once

#include "pass_base.h"
#include "function/render/render_model.h"

namespace MW {
    class RenderResourceBase;

    class MainCameraPass : public PassBase {
    public:
        void initialize(const RenderPassInitInfo *init_info) override;

        void preparePassData() override;

        void draw() override;

        virtual void updateAfterFramebufferRecreate();

    protected:
        VulkanBuffer cameraUniformBuffer;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        Model scene;
        void loadModel();

        virtual void createRenderPass();

        virtual void createPipelines();

        virtual void createDescriptorSets();

        virtual void createSwapchainFramebuffers();

        virtual void updateCamera();

        virtual void createUniformBuffer();
    };
} // namespace MW
