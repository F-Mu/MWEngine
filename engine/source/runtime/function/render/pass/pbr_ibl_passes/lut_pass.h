#pragma once

#include "runtime/function/render/pass/pass_base.h"

namespace MW {
    class VulkanTexture2D;
    struct LutPassInitInfo : public RenderPassInitInfo {
        int32_t imageDim{512};

        explicit LutPassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };

    class LutPass : public PassBase {
    public:
        void initialize(const RenderPassInitInfo *info) override;
        void draw() override;
        void clean() override;
        VulkanTexture2D lutTexture;
    private:
        void createRenderPass();

        void createFramebuffers();

        void createPipelines();

        void createLutTexture();

        static constexpr VkFormat lutFormat = VK_FORMAT_R16G16_SFLOAT;
        bool executed{false};
        int32_t imageDim{512};
    };
}