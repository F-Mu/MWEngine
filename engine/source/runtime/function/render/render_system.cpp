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
#include "window_system.h"

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
        mainCameraPass = std::make_shared<MainCameraPass>();
        mainCameraPass->initialize(&passInfo);
        renderCamera = std::make_shared<RenderCamera>();
        renderCamera->initialize();
        auto windowSize = info.window->getWindowSize();

        renderCamera->rotationSpeed *= 0.25f;
        renderCamera->setPerspective(45.0f, windowSize[0] * 1.0 / windowSize[1], 0.1f, 512.0f);
        renderCamera->setPosition(glm::vec3(0.0f, 1.0f, 0.0f));
        renderCamera->setRotation(glm::vec3(0.0f, -90.0f, 0.0f));
    }

    void RenderSystem::clean() {
        if (device) {
            device->clean();
            device.reset();
        }
    }

    void RenderSystem::tick(float delta_time) {
        renderCamera->update(delta_time);
        renderResource->rtData.projInverse = glm::inverse(renderCamera->matrices.perspective);
        renderResource->rtData.viewInverse = glm::inverse(renderCamera->matrices.view);
        static float timer = 0;
        float frameTime = 1;
        float timeSpeed = 0.01;
        timer += frameTime * timeSpeed * delta_time * 10;
        if (timer > 1)timer -= 1;
        glm ::vec4 lightPos = glm::vec4(cos(glm::radians(timer * 360.0f)) * 40.0f,
                                        -20.0f + sin(glm::radians(timer * 360.0f)) * 20.0f,
                                        25.0f + sin(glm::radians(timer * 360.0f)) * 5.0f, 0.0f);
        renderResource->rtData.lightPos = lightPos;
        renderResource->rtData.vertexSize = sizeof(gltfVertex);
        renderResource->cameraObject.projMatrix = renderCamera->matrices.perspective;
        renderResource->cameraObject.viewMatrix = renderCamera->matrices.view;
        renderResource->cameraObject.lightPos = lightPos;
        renderResource->cameraObject.viewPos = renderCamera->viewPos;
        mainCameraPass->preparePassData();
        device->prepareBeforePass(std::bind(&RenderSystem::passUpdateAfterRecreateSwapchain, this));
        mainCameraPass->draw();
        device->submitCommandBuffers(std::bind(&RenderSystem::passUpdateAfterRecreateSwapchain, this));
    }

    void RenderSystem::passUpdateAfterRecreateSwapchain() {
        mainCameraPass->updateAfterFramebufferRecreate();
    }
}