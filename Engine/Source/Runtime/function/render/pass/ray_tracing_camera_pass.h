#pragma once

#include "main_camera_pass.h"

namespace MW {
    class RayTracingCameraPass : public MainCameraPass {
    public:
        void initialize(const RenderPassInitInfo &init_info) override;

        void createDeviceSupport();

        void createBottomLevelAccelerationStructure();

        void createTopLevelAccelerationStructure();

        void createStorageImage();

        void createUniformBuffer();

        void createShaderBindingTable();

        void createDescriptorSets() override;

        void draw() override;

        void createPipelines() override;

        void updateAfterFramebufferRecreate() override;

        virtual void preparePassData() override;

    private:
        AccelerationStructure bottomLevelAS{};
        AccelerationStructure topLevelAS{};

        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;
        VulkanBuffer transformBuffer;
        VulkanBuffer raygenShaderBindingTable;
        VulkanBuffer missShaderBindingTable;
        VulkanBuffer hitShaderBindingTable;
        StorageImage storageImage;
        uint32_t indexCount;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
    };
}