//std
#include <stdexcept>
#include <array>
#include <cassert>
#include <iostream>
#include "rhi/vulkan_device.h"
#include "render_system.h"
#include "pass/main_camera_pass.h"
#include "pass/ray_tracing_camera_pass.h"
#include "render_resource.h"
#include "render_camera.h"

namespace MW {
    RenderSystem::RenderSystem() {
    }

    RenderSystem::~RenderSystem() {
    }

    void RenderSystem::initialize(RenderSystemInitInfo &info) {
        renderResource = std::make_shared<RenderResource>();
        VulkanDeviceInitInfo deviceInfo;
        deviceInfo.windowSystem = info.window;
        device = std::make_shared<VulkanDevice>();
        device->initialize(deviceInfo);
        RenderPassInitInfo passInfo;
        passInfo.device = device;
        passInfo.renderResource = renderResource;
        mainCameraPass = std::make_shared<RayTracingCameraPass>();
        mainCameraPass->initialize(passInfo);
        renderCamera = std::make_shared<RenderCamera>();
        auto x=glm::vec3();
    }

    void RenderSystem::clean() {
        if (device) {
            device->clean();
            device.reset();
        }
    }

    void RenderSystem::tick(float delta_time) {
        renderResource->cameraObject.projMatrix = glm::inverse(renderCamera->getProjection());
        renderResource->cameraObject.viewMatrix = glm::inverse(renderCamera->getView());
        mainCameraPass->preparePassData();
        device->prepareBeforePass(std::bind(&RenderSystem::passUpdateAfterRecreateSwapchain, this));
        mainCameraPass->draw();
        device->submitCommandBuffers(std::bind(&RenderSystem::passUpdateAfterRecreateSwapchain, this));
    }

    void RenderSystem::passUpdateAfterRecreateSwapchain() {
        mainCameraPass->updateAfterFramebufferRecreate();
    }
}