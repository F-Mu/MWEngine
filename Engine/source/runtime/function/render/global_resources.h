#pragma once

#include <vulkan/vulkan.h>
#include "render_descriptors.h"
#include "render_swap_chain.h"
#include "render_buffer.h"
#include "render_frame_info.h"
#include "rhi/vulkan_device.h"
namespace MW {
    class GlobalResources {
    public:
        GlobalResources(VulkanDevice &device);

        void clear();
        std::unique_ptr<RenderDescriptorPool> globalPool{};
        std::unique_ptr<RenderDescriptorSetLayout> globalSetLayout{};
        std::vector<std::unique_ptr<RenderBuffer>> uboBuffers;
        std::vector<VkDescriptorSet> globalDescriptorSets;
    };
}