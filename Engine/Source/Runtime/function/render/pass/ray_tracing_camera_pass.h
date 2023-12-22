#pragma once

#include "main_camera_pass.h"

namespace MW {
    class RayTracingCameraPass : MainCameraPass {
    public:
        void preparePassData(std::shared_ptr<RenderResource> render_resource) override;

        void createBuffer();
    private:
        VulkanBuffer cameraUniformBuffer;
    };
}