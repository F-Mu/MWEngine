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
        auto windowSize = info.window->getWindowSize();
        renderCamera->setAspect(windowSize[0] * 1.0 / windowSize[1]);
    }

    void RenderSystem::clean() {
        if (device) {
            device->clean();
            device.reset();
        }
    }

    void RenderSystem::tick(float delta_time) {
        renderResource->cameraUVWObject.camera_position = renderCamera->getPosition();
        renderResource->cameraUVWObject.lightPos = Vector4(40.0f, -20.0f , 25.0f ,0.0f); //TODO:修改
        // Pass the vertex size to the shader for unpacking vertices
        renderResource->cameraUVWObject.vertexSize = sizeof(gltfVertex);
        renderCamera->UVWFrame(renderResource->cameraUVWObject.camera_U,renderResource->cameraUVWObject.camera_V,renderResource->cameraUVWObject.camera_W);
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