#pragma once

//std
#include <cassert>
#include <memory>
#include <vector>
#include "rhi/vulkan_device.h"

namespace MW {
    class WindowSystem;

    class MainCameraPass;

    class RenderResource;

    class RenderCamera;

    class SceneManager;

    struct RenderSystemInitInfo {
        std::shared_ptr<WindowSystem> window;
    };

    class RenderSystem {
    public:
        RenderSystem();

        ~RenderSystem();

        void initialize(RenderSystemInitInfo &info);

        void clean();

        RenderSystem(const RenderSystem &) = delete;

        RenderSystem &operator=(const RenderSystem &) = delete;

        void tick(float delta_time);

        void passUpdateAfterRecreateSwapchain();

        void registerGarbageBuffer(VulkanBuffer& vulkanBuffer);

        std::shared_ptr<RenderCamera> getRenderCamera() { return renderCamera; }
        std::shared_ptr<SceneManager> getScene() { return sceneManager; }
        std::shared_ptr<VulkanDevice> device;
    private:
        void garbageCollection();
        std::vector<VulkanBuffer> deletedBuffer;
        std::shared_ptr<RenderResource> renderResource;
        std::shared_ptr<MainCameraPass> mainCameraPass;
        std::shared_ptr<WindowSystem> windowSystem;
        std::shared_ptr<SceneManager> sceneManager;
        std::shared_ptr<RenderCamera> renderCamera;
    };
}