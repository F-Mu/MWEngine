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
        mainCameraPass = std::make_shared<RayTracingCameraPass>();
        mainCameraPass->initialize(passInfo);
        renderCamera = std::make_shared<RenderCamera>();
        renderCamera->initialize();
        auto windowSize = info.window->getWindowSize();

        renderCamera->rotationSpeed *= 0.25f;
        renderCamera->setPerspective(45.0f, windowSize[0] * 1.0 / windowSize[1], 0.1f, 512.0f);
        renderCamera->setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
        renderCamera->setTranslation(glm::vec3(0.0f, 0.5f, -2.0f));
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
//        renderResource->cameraUVWObject.camera_position = renderCamera->getPosition();
        renderResource->rtData.lightPos = glm::vec4(cos(glm::radians(timer * 360.0f)) * 40.0f,
                                                    -20.0f + sin(glm::radians(timer * 360.0f)) * 20.0f,
                                                    25.0f + sin(glm::radians(timer * 360.0f)) * 5.0f, 0.0f);
//         Pass the vertex size to the shader for unpacking vertices
        renderResource->rtData.vertexSize = sizeof(gltfVertex);
//        renderCamera->UVWFrame(renderResource->cameraUVWObject.camera_U,renderResource->cameraUVWObject.camera_V,renderResource->cameraUVWObject.camera_W);
//        renderResource->cameraObject.projMatrix = renderCamera->getPersProjMatrix().inverse();
//        renderResource->cameraObject.viewMatrix = renderCamera->getViewMatrix().inverse();
//        {
//                std::cout<<renderCamera->forward()[0]<<' '<<renderCamera->forward()[1]<<'#'<<renderCamera->forward()[2]<<std::endl;
//                std::cout<<renderCamera->up()[0]<<' '<<renderCamera->up()[1]<<'#'<<renderCamera->up()[2]<<std::endl;
//                std::cout<<renderCamera->right()[0]<<' '<<renderCamera->right()[1]<<'#'<<renderCamera->right()[2]<<std::endl;
//                for(int i=0;i<4;++i){
//                    for(int j=0;j<4;++j)std::cout<<renderCamera->getPersProjMatrix()[i][j]<<' ';
//                    std::cout<<std::endl;
//                }
//                for(int i=0;i<4;++i){
//                    for(int j=0;j<4;++j)std::cout<<renderResource->cameraObject.viewMatrix[i][j]<<' ';
//                    std::cout<<std::endl;
//                }

//        }
        mainCameraPass->preparePassData();
        device->prepareBeforePass(std::bind(&RenderSystem::passUpdateAfterRecreateSwapchain, this));
        mainCameraPass->draw();
        device->submitCommandBuffers(std::bind(&RenderSystem::passUpdateAfterRecreateSwapchain, this));
    }

    void RenderSystem::passUpdateAfterRecreateSwapchain() {
        mainCameraPass->updateAfterFramebufferRecreate();
    }
}