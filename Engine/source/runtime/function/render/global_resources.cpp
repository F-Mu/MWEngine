#include "global_resources.h"

namespace MW {
    GlobalResources::GlobalResources(VulkanDevice &device) {
        globalPool = RenderDescriptorPool::Builder(device)
                .setMaxSets(RenderSwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, RenderSwapChain::MAX_FRAMES_IN_FLIGHT)
                .build();

        globalSetLayout = RenderDescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                .build();

        uboBuffers.resize(RenderSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); ++i) {
            uboBuffers[i] = std::make_unique<RenderBuffer>(
                    device,
                    sizeof(GlobalUbo),
                    1,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            uboBuffers[i]->map();
        }

        globalDescriptorSets.resize(RenderSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); ++i) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            RenderDescriptorWriter(*globalSetLayout, *globalPool)
                    .writeBuffer(0, &bufferInfo)
                    .build(globalDescriptorSets[i]);
        }
    }

    void GlobalResources::clear() {
        globalDescriptorSets.clear();
        uboBuffers.clear();
        globalSetLayout.reset();
        globalPool.reset();
    }
}