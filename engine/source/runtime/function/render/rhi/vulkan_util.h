#pragma once

#include <iostream>
#include <fstream>
#include <vulkan/vulkan.h>
#include <vector>

namespace MW {
    constexpr float M_E = 2.71828182845904523536;// e
    constexpr float M_LOG2E = 1.44269504088896340736;// log2(e)
    constexpr float M_LOG10E = 0.434294481903251827651;// log10(e)
    constexpr float M_LN2 = 0.693147180559945309417;// ln(2)
    constexpr float M_LN10 = 2.30258509299404568402;// ln(10)
    constexpr float M_PI = 3.14159265358979323846;// pi
    constexpr float M_PI_2 = 1.57079632679489661923;// pi/2
    constexpr float M_PI_4 = 0.785398163397448309616;// pi/4
    constexpr float M_1_PI = 0.318309886183790671538;// 1/pi
    constexpr float M_2_PI = 0.636619772367581343076;// 2/pi
    constexpr float M_2_SQRTPI = 1.12837916709551257390;// 2/sqrt(pi)
    constexpr float M_SQRT2 = 1.41421356237309504880;// sqrt(2)
    constexpr float M_SQRT1_2 = 0.707106781186547524401;// 1/sqrt(2)
    inline VkPushConstantRange CreatePushConstantRange(
            VkShaderStageFlags stageFlags,
            uint32_t size,
            uint32_t offset) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = stageFlags;
        pushConstantRange.offset = offset;
        pushConstantRange.size = size;
        return pushConstantRange;
    }

    inline VkPipelineColorBlendAttachmentState CreatePipelineColorBlendAttachmentState(
            VkColorComponentFlags colorWriteMask,
            VkBool32 blendEnable) {
        VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
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

    inline VkPipelineInputAssemblyStateCreateInfo CreatePipelineInputAssemblyStateCreateInfo(
            VkPrimitiveTopology topology,
            VkPipelineInputAssemblyStateCreateFlags flags,
            VkBool32 primitiveRestartEnable) {
        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
        pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipelineInputAssemblyStateCreateInfo.topology = topology;
        pipelineInputAssemblyStateCreateInfo.flags = flags;
        pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
        return pipelineInputAssemblyStateCreateInfo;
    }

    inline VkPipelineShaderStageCreateInfo
    loadShader(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStageFlagBits) {
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = shaderStageFlagBits;
        shaderStageInfo.module = shaderModule;
        shaderStageInfo.pName = "main";
        return shaderStageInfo;
    }

    inline VkGraphicsPipelineCreateInfo CreatePipelineCreateInfo(
            VkPipelineLayout layout,
            VkRenderPass renderPass,
            VkPipelineCreateFlags flags = 0) {
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout = layout;
        pipelineCreateInfo.renderPass = renderPass;
        pipelineCreateInfo.flags = flags;
        pipelineCreateInfo.basePipelineIndex = -1;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        return pipelineCreateInfo;
    }

    inline VkPipelineLayoutCreateInfo CreatePipelineLayoutCreateInfo(
            const VkDescriptorSetLayout *pSetLayouts,
            uint32_t setLayoutCount = 1) {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
        pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
        return pipelineLayoutCreateInfo;
    }

    inline VkVertexInputBindingDescription CreateVertexInputBindingDescription(
            uint32_t binding,
            uint32_t stride,
            VkVertexInputRate inputRate) {
        VkVertexInputBindingDescription vInputBindDescription{};
        vInputBindDescription.binding = binding;
        vInputBindDescription.stride = stride;
        vInputBindDescription.inputRate = inputRate;
        return vInputBindDescription;
    }

    inline VkVertexInputAttributeDescription CreateVertexInputAttributeDescription(
            uint32_t binding,
            uint32_t location,
            VkFormat format,
            uint32_t offset) {
        VkVertexInputAttributeDescription vInputAttribDescription{};
        vInputAttribDescription.location = location;
        vInputAttribDescription.binding = binding;
        vInputAttribDescription.format = format;
        vInputAttribDescription.offset = offset;
        return vInputAttribDescription;
    }

    inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo() {
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        return pipelineVertexInputStateCreateInfo;
    }

    inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
            const std::vector<VkVertexInputBindingDescription> &vertexBindingDescriptions,
            const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions
    ) {
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
        pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
        pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
        pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
        return pipelineVertexInputStateCreateInfo;
    }

    inline VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
            VkPrimitiveTopology topology,
            VkPipelineInputAssemblyStateCreateFlags flags,
            VkBool32 primitiveRestartEnable) {
        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
        pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipelineInputAssemblyStateCreateInfo.topology = topology;
        pipelineInputAssemblyStateCreateInfo.flags = flags;
        pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
        return pipelineInputAssemblyStateCreateInfo;
    }

    inline VkPipelineRasterizationStateCreateInfo CreatePipelineRasterizationStateCreateInfo(
            VkPolygonMode polygonMode,
            VkCullModeFlags cullMode,
            VkFrontFace frontFace,
            VkPipelineRasterizationStateCreateFlags flags = 0) {
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
        pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
        pipelineRasterizationStateCreateInfo.cullMode = cullMode;
        pipelineRasterizationStateCreateInfo.frontFace = frontFace;
        pipelineRasterizationStateCreateInfo.flags = flags;
        pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
        return pipelineRasterizationStateCreateInfo;
    }

    inline VkPipelineColorBlendStateCreateInfo CreatePipelineColorBlendStateCreateInfo(
            uint32_t attachmentCount,
            const VkPipelineColorBlendAttachmentState *pAttachments) {
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
        pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
        pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
        return pipelineColorBlendStateCreateInfo;
    }

    inline VkPipelineDepthStencilStateCreateInfo CreatePipelineDepthStencilStateCreateInfo(
            VkBool32 depthTestEnable,
            VkBool32 depthWriteEnable,
            VkCompareOp depthCompareOp) {
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
        pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
        pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
        pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
        pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
        return pipelineDepthStencilStateCreateInfo;
    }

    inline VkPipelineViewportStateCreateInfo CreatePipelineViewportStateCreateInfo(
            uint32_t viewportCount,
            uint32_t scissorCount,
            VkPipelineViewportStateCreateFlags flags = 0) {
        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
        pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipelineViewportStateCreateInfo.viewportCount = viewportCount;
        pipelineViewportStateCreateInfo.scissorCount = scissorCount;
        pipelineViewportStateCreateInfo.flags = flags;
        return pipelineViewportStateCreateInfo;
    }

    inline VkPipelineMultisampleStateCreateInfo CreatePipelineMultisampleStateCreateInfo(
            VkSampleCountFlagBits rasterizationSamples,
            VkPipelineMultisampleStateCreateFlags flags = 0) {
        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
        pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
        pipelineMultisampleStateCreateInfo.flags = flags;
        return pipelineMultisampleStateCreateInfo;
    }

    inline VkPipelineDynamicStateCreateInfo CreatePipelineDynamicStateCreateInfo(
            const VkDynamicState *pDynamicStates,
            uint32_t dynamicStateCount,
            VkPipelineDynamicStateCreateFlags flags = 0) {
        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
        pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
        pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
        pipelineDynamicStateCreateInfo.flags = flags;
        return pipelineDynamicStateCreateInfo;
    }

    inline VkPipelineDynamicStateCreateInfo CreatePipelineDynamicStateCreateInfo(
            const std::vector<VkDynamicState> &pDynamicStates,
            VkPipelineDynamicStateCreateFlags flags = 0) {
        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
        pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data();
        pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());
        pipelineDynamicStateCreateInfo.flags = flags;
        return pipelineDynamicStateCreateInfo;
    }

    inline VkPipelineVertexInputStateCreateInfo CreatePipelineVertexInputStateCreateInfo() {
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        return pipelineVertexInputStateCreateInfo;
    }

    inline VkDescriptorSetLayoutCreateInfo CreateDescriptorSetLayoutCreateInfo(
            const VkDescriptorSetLayoutBinding *pBindings,
            uint32_t bindingCount) {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pBindings = pBindings;
        descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
        return descriptorSetLayoutCreateInfo;
    }

    inline VkDescriptorSetLayoutCreateInfo CreateDescriptorSetLayoutCreateInfo(
            const std::vector<VkDescriptorSetLayoutBinding> &bindings) {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pBindings = bindings.data();
        descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        return descriptorSetLayoutCreateInfo;
    }

    inline VkWriteDescriptorSet CreateWriteDescriptorSet(
            VkDescriptorSet dstSet,
            VkDescriptorType type,
            uint32_t binding,
            VkDescriptorBufferInfo *bufferInfo,
            uint32_t descriptorCount = 1) {
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pBufferInfo = bufferInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;
        return writeDescriptorSet;
    }

    inline VkDescriptorImageInfo
    CreateDescriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout) {
        VkDescriptorImageInfo descriptorImageInfo{};
        descriptorImageInfo.sampler = sampler;
        descriptorImageInfo.imageView = imageView;
        descriptorImageInfo.imageLayout = imageLayout;
        return descriptorImageInfo;
    }

    inline VkWriteDescriptorSet CreateWriteDescriptorSet(
            VkDescriptorSet dstSet,
            VkDescriptorType type,
            uint32_t binding,
            VkDescriptorImageInfo *imageInfo,
            uint32_t descriptorCount = 1) {
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pImageInfo = imageInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;
        return writeDescriptorSet;
    }

    inline VkImageCreateInfo CreateImageCreateInfo() {
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        return imageCreateInfo;
    }

    inline VkFramebufferCreateInfo CreateFramebufferCreateInfo() {
        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        return framebufferCreateInfo;
    }

    inline VkImageViewCreateInfo CreateImageViewCreateInfo() {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        return imageViewCreateInfo;
    }

    inline VkSamplerCreateInfo CreateSamplerCreateInfo() {
        VkSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.maxAnisotropy = 1.0f;
        return samplerCreateInfo;
    }

    inline VkRenderPassCreateInfo CreateRenderPassCreateInfo() {
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        return renderPassCreateInfo;
    }

    inline VkRenderPassBeginInfo CreateRenderPassBeginInfo() {
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        return renderPassBeginInfo;
    }

    inline VkViewport CreateViewport(
            float width,
            float height,
            float minDepth,
            float maxDepth) {
        VkViewport viewport{};
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = minDepth;
        viewport.maxDepth = maxDepth;
        return viewport;
    }

    inline VkRect2D CreateRect2D(
            int32_t width,
            int32_t height,
            int32_t offsetX,
            int32_t offsetY) {
        VkRect2D rect2D{};
        rect2D.extent.width = width;
        rect2D.extent.height = height;
        rect2D.offset.x = offsetX;
        rect2D.offset.y = offsetY;
        return rect2D;
    }

    /** @brief Initialize a map entry for a shader specialization constant */
    inline VkSpecializationMapEntry
    CreateSpecializationMapEntry(uint32_t constantID, uint32_t offset, size_t size) {
        VkSpecializationMapEntry specializationMapEntry{};
        specializationMapEntry.constantID = constantID;
        specializationMapEntry.offset = offset;
        specializationMapEntry.size = size;
        return specializationMapEntry;
    }

    /** @brief Initialize a specialization constant info structure to pass to a shader stage */
    inline VkSpecializationInfo
    CreateSpecializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry *mapEntries, size_t dataSize,
                             const void *data) {
        VkSpecializationInfo specializationInfo{};
        specializationInfo.mapEntryCount = mapEntryCount;
        specializationInfo.pMapEntries = mapEntries;
        specializationInfo.dataSize = dataSize;
        specializationInfo.pData = data;
        return specializationInfo;
    }
    inline VkFenceCreateInfo CreateFenceCreateInfo(VkFenceCreateFlags flags = 0)
    {
        VkFenceCreateInfo fenceCreateInfo {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = flags;
        return fenceCreateInfo;
    }
    const std::string getAssetPath();

    bool fileExists(const std::string &filename);

    void exitFatal(const std::string &message, int32_t exitCode);

    class VulkanUtil {
    public:
        static uint32_t alignedSize(uint32_t value, uint32_t alignment);
    };
} // namespace MW
