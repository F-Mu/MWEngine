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
        renderResource->cameraObject.projMatrix = renderCamera->getPersProjMatrix().inverse();
        renderResource->cameraObject.viewMatrix = renderCamera->getViewMatrix().inverse();
        {
            static bool first =true;
            if(first){
                first=false;
                std::cout<<renderCamera->forward()[0]<<' '<<renderCamera->forward()[1]<<'#'<<renderCamera->forward()[2]<<std::endl;
                std::cout<<renderCamera->up()[0]<<' '<<renderCamera->up()[1]<<'#'<<renderCamera->up()[2]<<std::endl;
                std::cout<<renderCamera->right()[0]<<' '<<renderCamera->right()[1]<<'#'<<renderCamera->right()[2]<<std::endl;
                for(int i=0;i<4;++i){
                    for(int j=0;j<4;++j)std::cout<<renderCamera->getPersProjMatrix()[i][j]<<' ';
                    std::cout<<std::endl;
                }
                for(int i=0;i<4;++i){
                    for(int j=0;j<4;++j)std::cout<<renderCamera->getViewMatrix()[i][j]<<' ';
                    std::cout<<std::endl;
                }
            }
        }
        mainCameraPass->preparePassData();
        device->prepareBeforePass(std::bind(&RenderSystem::passUpdateAfterRecreateSwapchain, this));
        mainCameraPass->draw();
        device->submitCommandBuffers(std::bind(&RenderSystem::passUpdateAfterRecreateSwapchain, this));
    }

    void RenderSystem::passUpdateAfterRecreateSwapchain() {
        mainCameraPass->updateAfterFramebufferRecreate();
    }
}