# pragma once

#include "main_camera_pass.h"

namespace MW {

    class MainCameraPass2 : public MainCameraPass {
#if USE_VRS
    protected:
        void initialize(const RenderPassInitInfo *init_info) override;

        virtual void createRenderPass() override;

        virtual void draw() override;

        void createSwapchainFramebuffers() override;

        template<size_t N>
        void setupAttachment2KHRType(std::array<VkAttachmentReference2KHR, N>& attachmentReferences);

        VkExtent2D fragmentShadingRateSize = {1, 1};
        // Set the fragment shading rate state for the current pipeline
        // The combiners determine how the different shading rate values for the pipeline, primitives and attachment are combined
        VkFragmentShadingRateCombinerOpKHR combinerOps[2] = {
                VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
                VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR
        };

        glm::ivec2 shadingRateIndex{0, 0};
        glm::ivec2 shadingRateCombinerIndex{0, 1};

        void UpdateUIOverlay(UIOverlay *overlay);

#endif
    };

} // MW

