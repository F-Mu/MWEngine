#pragma once

#include "main_camera_pass.h"
#include "function/render/render_model.h"
namespace MW {
    class RayTracingCameraPass : public MainCameraPass {
    public:
        void initialize(const RenderPassInitInfo &init_info) override;

        void createBottomLevelAccelerationStructure();

        void createTopLevelAccelerationStructure();

        void createStorageImage();

        void createUniformBuffer() override;

        void createShaderBindingTable();

        void createDescriptorSets() override;

        void draw() override;

        void createPipelines() override;

        void updateAfterFramebufferRecreate() override;

        void preparePassData() override;

        void updateCamera() override;

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