#pragma once

#include <iostream>
#include <fstream>
#include <vulkan/vulkan.h>

namespace MW {
    inline VkPushConstantRange CreatePushConstantRange(
            VkShaderStageFlags stageFlags,
            uint32_t size,
            uint32_t offset)
    {
        VkPushConstantRange pushConstantRange {};
        pushConstantRange.stageFlags = stageFlags;
        pushConstantRange.offset = offset;
        pushConstantRange.size = size;
        return pushConstantRange;
    }
    inline VkPipelineColorBlendAttachmentState CreatePipelineColorBlendAttachmentState(
            VkColorComponentFlags colorWriteMask,
            VkBool32 blendEnable)
    {
        VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState {};
        pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
        pipelineColorBlendAttachmentState.blendEnable = blendEnable;
        return pipelineColorBlendAttachmentState;
    }
    inline VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(
            VkDescriptorType type,
            VkShaderStageFlags stageFlags,
            uint32_t binding,
            uint32_t descriptorCount = 1) {
        VkDescriptorSetLayoutBinding setLayoutBinding{};
        setLayoutBinding.descriptorType = type;
        setLayoutBinding.stageFlags = stageFlags;
        setLayoutBinding.binding = binding;
        setLayoutBinding.descriptorCount = descriptorCount;
        return setLayoutBinding;
    }

    inline VkImageCreateInfo CreateImageCreateInfo() {
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        return imageCreateInfo;
    }
    /** @brief Initialize a map entry for a shader specialization constant */
    inline VkSpecializationMapEntry CreateSpecializationMapEntry(uint32_t constantID, uint32_t offset, size_t size)
    {
        VkSpecializationMapEntry specializationMapEntry{};
        specializationMapEntry.constantID = constantID;
        specializationMapEntry.offset = offset;
        specializationMapEntry.size = size;
        return specializationMapEntry;
    }
    /** @brief Initialize a specialization constant info structure to pass to a shader stage */
    inline VkSpecializationInfo CreateSpecializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
    {
        VkSpecializationInfo specializationInfo{};
        specializationInfo.mapEntryCount = mapEntryCount;
        specializationInfo.pMapEntries = mapEntries;
        specializationInfo.dataSize = dataSize;
        specializationInfo.pData = data;
        return specializationInfo;
    }

    const std::string getAssetPath();

    bool fileExists(const std::string &filename);

    void exitFatal(const std::string &message, int32_t exitCode);
    class VulkanUtil {
    public:
        static uint32_t alignedSize(uint32_t value, uint32_t alignment);
    };
} // namespace MW
