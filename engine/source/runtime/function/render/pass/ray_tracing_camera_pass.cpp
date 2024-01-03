#include "ray_tracing_camera_pass.h"
#include "function/render/render_resource.h"
#include "raygen_rgen.h"
#include "miss_rmiss.h"
#include "shadow_rmiss.h"
#include "closesthit_rchit.h"

namespace MW {
    void RayTracingCameraPass::initialize(const RenderPassInitInfo &init_info) {
        PassBase::initialize(init_info);
        createBottomLevelAccelerationStructure();
        createTopLevelAccelerationStructure();

        createStorageImage();
        createUniformBuffer();
        createDescriptorSets();
        createPipelines();
        createShaderBindingTable();
    }

    void RayTracingCameraPass::createBottomLevelAccelerationStructure() {
        // Instead of a simple triangle, we'll be loading a more complex scene for this example
        // The shaders are accessing the vertex and index buffers of the scene, so the proper usage flag has to be set on the vertex and index buffers for the scene
        memoryPropertyFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        const uint32_t glTFLoadingFlags =
                FileLoadingFlags::PreTransformVertices | FileLoadingFlags::PreMultiplyVertexColors |
                FileLoadingFlags::FlipY;
        scene.loadFromFile(getAssetPath() + "models/oaktree.gltf", device.get(), glTFLoadingFlags);

        VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
        VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

        vertexBufferDeviceAddress.deviceAddress = device->getBufferDeviceAddress(scene.vertices.buffer.buffer);
        indexBufferDeviceAddress.deviceAddress = device->getBufferDeviceAddress(scene.indices.buffer.buffer);

        uint32_t numTriangles = static_cast<uint32_t>(scene.indices.count) / 3;

//        VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

//        vertexBufferDeviceAddress.deviceAddress = device->getBufferDeviceAddress(vertexBuffer.buffer);
//        indexBufferDeviceAddress.deviceAddress = device->getBufferDeviceAddress(indexBuffer.buffer);
//        transformBufferDeviceAddress.deviceAddress = device->getBufferDeviceAddress(transformBuffer.buffer);

        // Build
        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
        accelerationStructureGeometry.geometry.triangles.maxVertex = 3;
        accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(gltfVertex);
        accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
        accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
        accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
//        accelerationStructureGeometry.geometry.triangles.transformData = transformBufferDeviceAddress;

        // Get size info
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        device->GetAccelerationStructureBuildSizesKHR(
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &accelerationStructureBuildGeometryInfo,
                &numTriangles,
                &accelerationStructureBuildSizesInfo);

        device->createAccelerationStructureBuffer(bottomLevelAS, accelerationStructureBuildSizesInfo);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = bottomLevelAS.buffer;
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        device->CreateAccelerationStructureKHR(&accelerationStructureCreateInfo, &bottomLevelAS.handle);

        // Create a small scratch buffer used during build of the bottom level acceleration structure
        RayTracingScratchBuffer scratchBuffer = device->createScratchBuffer(
                accelerationStructureBuildSizesInfo.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
        accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.handle;
        accelerationBuildGeometryInfo.geometryCount = 1;
        accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
        accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
        accelerationStructureBuildRangeInfo.primitiveOffset = 0;
        accelerationStructureBuildRangeInfo.firstVertex = 0;
        accelerationStructureBuildRangeInfo.transformOffset = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {
                &accelerationStructureBuildRangeInfo};

        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
        device->CmdBuildAccelerationStructuresKHR(
                commandBuffer,
                1,
                &accelerationBuildGeometryInfo,
                accelerationBuildStructureRangeInfos.data());
        device->endSingleTimeCommands(commandBuffer);

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = bottomLevelAS.handle;
        bottomLevelAS.deviceAddress = device->getAccelerationStructureDeviceAddressKHR(&accelerationDeviceAddressInfo);

        device->destroyScratchBuffer(scratchBuffer);
    }

    void RayTracingCameraPass::createTopLevelAccelerationStructure() {
        VkTransformMatrixKHR transformMatrix = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f};

        VkAccelerationStructureInstanceKHR instance{};
        instance.transform = transformMatrix;
        instance.instanceCustomIndex = 0;
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = bottomLevelAS.deviceAddress;

        // Buffer for instance data
        VulkanBuffer instancesBuffer;
        device->CreateBuffer(
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                instancesBuffer,
                sizeof(VkAccelerationStructureInstanceKHR),
                &instance);

        VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
        instanceDataDeviceAddress.deviceAddress = device->getBufferDeviceAddress(instancesBuffer.buffer);

        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
        accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

        // Get size info
        /*
        The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored. Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that the hostAddress member of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be examined to check if it is NULL.*
        */
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        uint32_t primitive_count = 1;

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        device->GetAccelerationStructureBuildSizesKHR(
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &accelerationStructureBuildGeometryInfo,
                &primitive_count,
                &accelerationStructureBuildSizesInfo);

        device->createAccelerationStructureBuffer(topLevelAS, accelerationStructureBuildSizesInfo);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = topLevelAS.buffer;
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        device->CreateAccelerationStructureKHR(&accelerationStructureCreateInfo, &topLevelAS.handle);

        // Create a small scratch buffer used during build of the top level acceleration structure
        RayTracingScratchBuffer scratchBuffer = device->createScratchBuffer(
                accelerationStructureBuildSizesInfo.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
        accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.dstAccelerationStructure = topLevelAS.handle;
        accelerationBuildGeometryInfo.geometryCount = 1;
        accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
        accelerationStructureBuildRangeInfo.primitiveCount = 1;
        accelerationStructureBuildRangeInfo.primitiveOffset = 0;
        accelerationStructureBuildRangeInfo.firstVertex = 0;
        accelerationStructureBuildRangeInfo.transformOffset = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {
                &accelerationStructureBuildRangeInfo};

        // Build the acceleration structure on the device via a one-time command buffer submission
        // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
        device->CmdBuildAccelerationStructuresKHR(
                commandBuffer,
                1,
                &accelerationBuildGeometryInfo,
                accelerationBuildStructureRangeInfos.data());
        device->endSingleTimeCommands(commandBuffer);

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = topLevelAS.handle;
        topLevelAS.deviceAddress = device->getAccelerationStructureDeviceAddressKHR(&accelerationDeviceAddressInfo);

        device->destroyScratchBuffer(scratchBuffer);
        device->destroyVulkanBuffer(instancesBuffer);
    }

    void RayTracingCameraPass::createStorageImage() {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = device->getSwapchainInfo().imageFormat;
        imageInfo.extent.width = device->getSwapchainInfo().extent.width;
        imageInfo.extent.height = device->getSwapchainInfo().extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        device->CreateImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, storageImage.image,
                                    storageImage.memory);

        VkImageViewCreateInfo colorImageViewInfo{};
        colorImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorImageViewInfo.format = device->getSwapchainInfo().imageFormat;
        colorImageViewInfo.subresourceRange = {};
        colorImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorImageViewInfo.subresourceRange.baseMipLevel = 0;
        colorImageViewInfo.subresourceRange.levelCount = 1;
        colorImageViewInfo.subresourceRange.baseArrayLayer = 0;
        colorImageViewInfo.subresourceRange.layerCount = 1;
        colorImageViewInfo.image = storageImage.image;
        device->CreateImageView(&colorImageViewInfo, &storageImage.view);

        device->transitionImageLayout(storageImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    }

    void RayTracingCameraPass::createUniformBuffer() {
        device->CreateBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                cameraUniformBuffer,
                sizeof(renderResource->rtData),
                &renderResource->rtData);
        device->MapMemory(cameraUniformBuffer);

        memcpy(cameraUniformBuffer.mapped, &renderResource->rtData, sizeof(renderResource->rtData));
    }

    void RayTracingCameraPass::createShaderBindingTable() {
        const uint32_t handleSize = device->rayTracingPipelineProperties.shaderGroupHandleSize;
        const uint32_t handleSizeAligned = VulkanUtil::alignedSize(
                device->rayTracingPipelineProperties.shaderGroupHandleSize,
                device->rayTracingPipelineProperties.shaderGroupHandleAlignment);
        const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
        const uint32_t sbtSize = groupCount * handleSizeAligned;

        std::vector<uint8_t> shaderHandleStorage(sbtSize);
        device->GetRayTracingShaderGroupHandlesKHR(pipelines[0].pipeline, 0, groupCount, sbtSize,
                                                   shaderHandleStorage.data());

        const VkBufferUsageFlags bufferUsageFlags =
                VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        const VkMemoryPropertyFlags memoryUsageFlags =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        device->CreateBuffer(bufferUsageFlags, memoryUsageFlags, raygenShaderBindingTable, handleSize);
        device->CreateBuffer(bufferUsageFlags, memoryUsageFlags, missShaderBindingTable, handleSize * 2);
        device->CreateBuffer(bufferUsageFlags, memoryUsageFlags, hitShaderBindingTable, handleSize);

        // Copy handles

        device->MapMemory(raygenShaderBindingTable);
        device->MapMemory(missShaderBindingTable);
        device->MapMemory(hitShaderBindingTable);
        memcpy(raygenShaderBindingTable.mapped, shaderHandleStorage.data(), handleSize);
        memcpy(missShaderBindingTable.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
        memcpy(hitShaderBindingTable.mapped, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);
    }

    void RayTracingCameraPass::createDescriptorSets() {
        descriptors.resize(1);
        VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
        accelerationStructureLayoutBinding.binding = 0;
        accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureLayoutBinding.descriptorCount = 1;
        accelerationStructureLayoutBinding.stageFlags =
                VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
        resultImageLayoutBinding.binding = 1;
        resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        resultImageLayoutBinding.descriptorCount = 1;
        resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding uniformBufferBinding{};
        uniformBufferBinding.binding = 2;
        uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferBinding.descriptorCount = 1;
        uniformBufferBinding.stageFlags =
                VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

        VkDescriptorSetLayoutBinding vertexBufferBinding{};
        vertexBufferBinding.binding = 3;
        vertexBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vertexBufferBinding.descriptorCount = 1;
        vertexBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding indexBufferBinding{};
        indexBufferBinding.binding = 4;
        indexBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        indexBufferBinding.descriptorCount = 1;
        indexBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        std::vector<VkDescriptorSetLayoutBinding> bindings({accelerationStructureLayoutBinding,
                                                            resultImageLayoutBinding,
                                                            uniformBufferBinding,
                                                            vertexBufferBinding,
                                                            indexBufferBinding
                                                           });

        VkDescriptorSetLayoutCreateInfo descriptorSetlayoutCI{};
        descriptorSetlayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetlayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetlayoutCI.pBindings = bindings.data();
        device->CreateDescriptorSetLayout(&descriptorSetlayoutCI, &descriptors[0].layout);

        device->CreateDescriptorSet(1, descriptors[0].layout, descriptors[0].descriptorSet);

        VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
        descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.handle;

        VkDescriptorImageInfo storageImageDescriptor{};
        storageImageDescriptor.imageView = storageImage.view;
        storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = cameraUniformBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraRTData);
        VkDescriptorBufferInfo vertexBufferDescriptor{scene.vertices.buffer.buffer, 0, VK_WHOLE_SIZE};
        VkDescriptorBufferInfo indexBufferDescriptor{scene.indices.buffer.buffer, 0, VK_WHOLE_SIZE};

        std::array<VkWriteDescriptorSet, 5> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // The specialized acceleration structure descriptor has to be chained
        descriptorWrites[0].pNext = &descriptorAccelerationStructureInfo;
        descriptorWrites[0].dstSet = descriptors[0].descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptors[0].descriptorSet;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].pImageInfo = &storageImageDescriptor;
        descriptorWrites[1].descriptorCount = 1;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptors[0].descriptorSet;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].pBufferInfo = &bufferInfo;
        descriptorWrites[2].descriptorCount = 1;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = descriptors[0].descriptorSet;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].pBufferInfo = &vertexBufferDescriptor;
        descriptorWrites[3].descriptorCount = 1;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = descriptors[0].descriptorSet;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].pBufferInfo = &indexBufferDescriptor;
        descriptorWrites[4].descriptorCount = 1;
        device->UpdateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data());
    }

    void RayTracingCameraPass::createPipelines() {
        pipelines.resize(1);
        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts = &descriptors[0].layout;
        device->CreatePipelineLayout(&pipelineLayoutCI, &pipelines[0].layout);
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};

        // Ray generation group
        {
            VkSpecializationMapEntry specializationMapEntry = CreateSpecializationMapEntry(0, 0, sizeof(uint32_t));
            uint32_t maxRecursion = 4;
            VkSpecializationInfo specializationInfo = CreateSpecializationInfo(1, &specializationMapEntry,
                                                                               sizeof(maxRecursion), &maxRecursion);

            auto raygenShaderModule = device->CreateShaderModule(RAYGEN_RGEN);
            VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
            shaderStageCreateInfo.pSpecializationInfo = &specializationInfo;
            shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageCreateInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            shaderStageCreateInfo.module = raygenShaderModule;
            shaderStageCreateInfo.pName = "main";
            shaderStages.emplace_back(shaderStageCreateInfo);

            VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
            shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
            shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.push_back(shaderGroup);
        }

        // Miss group
        {
            auto missShaderModule = device->CreateShaderModule(MISS_RMISS);
            VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
            shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageCreateInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
            shaderStageCreateInfo.module = missShaderModule;
            shaderStageCreateInfo.pName = "main";
            shaderStages.emplace_back(shaderStageCreateInfo);

            VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
            shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
            shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.push_back(shaderGroup);

            missShaderModule = device->CreateShaderModule(SHADOW_RMISS);
            shaderStageCreateInfo.module = missShaderModule;
            shaderStages.emplace_back(shaderStageCreateInfo);
            shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
            shaderGroups.push_back(shaderGroup);
        }

        // Closest hit group
        {
            auto closestHitShaderModule = device->CreateShaderModule(CLOSESTHIT_RCHIT);
            VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
            shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageCreateInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            shaderStageCreateInfo.module = closestHitShaderModule;
            shaderStageCreateInfo.pName = "main";
            shaderStages.emplace_back(shaderStageCreateInfo);

            VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
            shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.push_back(shaderGroup);

        }

        /*
            Create the ray tracing pipeline
        */
        VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
        rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        rayTracingPipelineCI.pStages = shaderStages.data();
        rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
        rayTracingPipelineCI.pGroups = shaderGroups.data();
        rayTracingPipelineCI.maxPipelineRayRecursionDepth = 2;
        rayTracingPipelineCI.layout = pipelines[0].layout;
        device->CreateRayTracingPipelinesKHR(&rayTracingPipelineCI, &pipelines[0].pipeline);
    }

    void RayTracingCameraPass::draw() {
        /*
            Setup the buffer regions pointing to the shaders in our shader binding table
        */

        const uint32_t handleSizeAligned = VulkanUtil::alignedSize(
                device->rayTracingPipelineProperties.shaderGroupHandleSize,
                device->rayTracingPipelineProperties.shaderGroupHandleAlignment);

        VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
        raygenShaderSbtEntry.deviceAddress = device->getBufferDeviceAddress(raygenShaderBindingTable.buffer);
        raygenShaderSbtEntry.stride = handleSizeAligned;
        raygenShaderSbtEntry.size = handleSizeAligned;

        VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
        missShaderSbtEntry.deviceAddress = device->getBufferDeviceAddress(missShaderBindingTable.buffer);
        missShaderSbtEntry.stride = handleSizeAligned;
        missShaderSbtEntry.size = handleSizeAligned;

        VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
        hitShaderSbtEntry.deviceAddress = device->getBufferDeviceAddress(hitShaderBindingTable.buffer);
        hitShaderSbtEntry.stride = handleSizeAligned;
        hitShaderSbtEntry.size = handleSizeAligned;

        VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

        /*
            Dispatch the ray tracing commands
        */
        vkCmdBindPipeline(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                          pipelines[0].pipeline);
        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                                pipelines[0].layout, 0, 1,
                                &descriptors[0].descriptorSet, 0, 0);

        device->CmdTraceRaysKHR(
                device->getCurrentCommandBuffer(),
                &raygenShaderSbtEntry,
                &missShaderSbtEntry,
                &hitShaderSbtEntry,
                &callableShaderSbtEntry,
                device->getSwapchainInfo().extent.width,
                device->getSwapchainInfo().extent.height,
                1);

        /*
            Copy ray tracing output to swap chain image
        */

        // Prepare current swap chain image as transfer destination

        VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        device->transitionImageLayout(
                device->getCurrentImage(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, true, subresourceRange);

        // Prepare ray tracing output image as transfer source
        device->transitionImageLayout(
                storageImage.image,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, true, subresourceRange);

        //DENOISE
        denoise();

        VkImageCopy copyRegion{};
        copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.srcOffset = {0, 0, 0};
        copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.dstOffset = {0, 0, 0};
        copyRegion.extent = {device->getSwapchainInfo().extent.width,
                             device->getSwapchainInfo().extent.height,
                             1};
        vkCmdCopyImage(device->getCurrentCommandBuffer(), storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       device->getCurrentImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        // Transition swap chain image back for presentation
        device->transitionImageLayout(
                device->getCurrentImage(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, true,
                subresourceRange);

        // Transition ray tracing output image back to general layout
        device->transitionImageLayout(
                storageImage.image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_GENERAL, true,
                subresourceRange);
    }

    void RayTracingCameraPass::updateAfterFramebufferRecreate() {

        // Delete allocated resources
        device->DestroyImageView(storageImage.view);
        device->DestroyImage(storageImage.image);
        device->FreeMemory(storageImage.memory);
        // Recreate image
        createStorageImage();
        // Update descriptor
        VkDescriptorImageInfo storageImageDescriptor{VK_NULL_HANDLE, storageImage.view, VK_IMAGE_LAYOUT_GENERAL};
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptors[0].descriptorSet;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSet.dstBinding = 1;
        writeDescriptorSet.pImageInfo = &storageImageDescriptor;
        writeDescriptorSet.descriptorCount = 1;
        device->UpdateDescriptorSets(1, &writeDescriptorSet);
    }

    void RayTracingCameraPass::preparePassData() {
        updateCamera();
    }

    void RayTracingCameraPass::updateCamera() {
        memcpy(cameraUniformBuffer.mapped, &renderResource->rtData, sizeof(renderResource->rtData));
    }

    void RayTracingCameraPass::denoise() {

    }
}