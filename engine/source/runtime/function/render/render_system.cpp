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
#include "scene_manager.h"

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
        sceneManager = std::make_shared<SceneManager>();
        SceneManagerInitInfo sceneInfo;
        sceneInfo.device = device;
        sceneManager->initialize(&sceneInfo);
        renderCamera = std::make_shared<RenderCamera>();
        renderCamera->initialize();
        auto windowSize = info.window->getWindowSize();
        renderCamera->rotationSpeed *= 0.25f;
        renderCamera->setPerspective(45.0f, windowSize[0] * 1.0 / windowSize[1], 0.1f, 256.0f);
        renderCamera->setPosition(glm::vec3(-0.12f, 1.14f, -2.25f));
        renderCamera->setRotation(glm::vec3(-17.0f, 7.0f, 0.0f));
        RenderPassInitInfo passInfo;
        passInfo.device = device;
        passInfo.renderResource = renderResource;
        mainCameraPass = std::make_shared<MainCameraPass>();
        mainCameraPass->initialize(&passInfo);
    }

    void RenderSystem::clean() {
        renderCamera.reset();
        mainCameraPass.reset();
        sceneManager.reset();
        if (device) {
            device->clean();
            device.reset();
        }
    }

    void RenderSystem::tick(float delta_time) {
        static float timer = 0;
        float frameTime = 1;
        float timeSpeed = 0.00;
        timer += frameTime * timeSpeed * delta_time * 10;
        if (timer > 1)timer -= 1;
        float angle = glm::radians(timer * 360.0f);
        float radius = 20.0f;
        glm::vec4 lightPos = glm::vec4(cos(angle) * radius, -radius, sin(angle) * radius, 0.0f);
        renderCamera->update(delta_time);
        renderResource->rtData.projInverse = glm::inverse(renderCamera->matrices.perspective);
        renderResource->rtData.viewInverse = glm::inverse(renderCamera->matrices.view);
        renderResource->rtData.lightPos = lightPos;
        renderResource->rtData.vertexSize = sizeof(gltfVertex);
        renderResource->cameraObject.projMatrix = renderCamera->matrices.perspective;
        renderResource->cameraObject.viewMatrix = renderCamera->matrices.view;
        renderResource->cameraObject.projViewMatrix =
                renderResource->cameraObject.projMatrix * renderResource->cameraObject.viewMatrix;
        renderResource->cameraObject.lightPos = lightPos;
        renderResource->cameraObject.viewPos = renderCamera->viewPos;
        renderResource->cameraObject.position = renderCamera->position;
        mainCameraPass->preparePassData();
        device->prepareBeforePass(std::bind(&RenderSystem::passUpdateAfterRecreateSwapchain, this));
        mainCameraPass->draw();
        device->submitCommandBuffers(std::bind(&RenderSystem::passUpdateAfterRecreateSwapchain, this));
    }

    void RenderSystem::passUpdateAfterRecreateSwapchain() {
        mainCameraPass->updateAfterFramebufferRecreate();
    }
}