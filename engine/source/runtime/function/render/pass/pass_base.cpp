#include "pass_base.h"

namespace MW {
    void PassBase::initialize(const RenderPassInitInfo *init_info) {
        device = init_info->device;
        renderResource = init_info->renderResource;
    }

    void PassBase::preparePassData() {}

    void PassBase::draw() {}

    VkRenderPass PassBase::getRenderPass() const { return framebuffer.renderPass; }

    std::vector<VkImageView> PassBase::getFramebufferImageViews() const {
        std::vector<VkImageView> image_views;
        for (auto &attach: framebuffer.attachments) {
            image_views.push_back(attach.view);
        }
        return image_views;
    }

    std::vector<VkDescriptorSetLayout> PassBase::getDescriptorSetLayouts() const {
        std::vector<VkDescriptorSetLayout> layouts;
        for (auto &desc: descriptors) {
            layouts.push_back(desc.layout);
        }
        return layouts;
    }
} // namespace MW
