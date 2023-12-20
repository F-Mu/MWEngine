#pragma once

#include "render_camera.h"
#include "resources/manager/game_object_manager.h"

#include <vulkan/vulkan.h>

namespace MW {

    struct GlobalUbo {
        alignas(16) glm::mat4 projection{1.f};
        alignas(16) glm::mat4 view{1.f};
        alignas(16) glm::mat4 inverseView{1.f};
    };

    struct RenderFrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        VkDescriptorSet globalDescriptorSet;
        GameObjectManager::Map &gameObjects;
    };
}