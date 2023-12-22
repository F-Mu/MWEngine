#include "pass_base.h"

namespace MW {
    void PassBase::initialize(const RenderPassInitInfo *init_info) {
        device = init_info->device;
    }

    void PassBase::draw() {}

    VkRenderPass *PassBase::getRenderPass() const { return m_framebuffer.render_pass; }

    std::vector<VkImageView *> PassBase::getFramebufferImageViews() const {
        std::vector<VkImageView *> image_views;
        for (auto &attach: m_framebuffer.attachments) {
            image_views.push_back(attach.view);
        }
        return image_views;
    }

    std::vector<VkDescriptorSetLayout *> PassBase::getDescriptorSetLayouts() const {
        std::vector<VkDescriptorSetLayout *> layouts;
        for (auto &desc: m_descriptor_infos) {
            layouts.push_back(desc.layout);
        }
        return layouts;
    }
} // namespace MW
