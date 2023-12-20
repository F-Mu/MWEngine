//std
#include <stdexcept>
#include <array>
#include <cassert>
#include <iostream>
#include "rhi/vulkan_device.h"
#include "render_system.h"

namespace MW {
    RenderSystem::RenderSystem(RenderSystemInitInfo info){
        VulkanDeviceInitInfo deviceInfo;
        deviceInfo.windowSystem = info.window;
        vulkanDevice = std::make_shared<VulkanDevice>(deviceInfo);

    }

    RenderSystem::~RenderSystem() {
    }

}