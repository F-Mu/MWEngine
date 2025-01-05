
/*
* UI overlay class using ImGui
*
* Copyright (C) 2017-2024 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanUIOverlay.h"
#include "uioverlay_frag.h"
#include "uioverlay_vert.h"
#include "function/render/render_system.h"
#include "function/global/engine_global_context.h"
namespace MW {
    UIOverlay::UIOverlay() {
        // Init ImGui
        ImGui::CreateContext();
        // Color scheme
        ImGuiStyle &style = ImGui::GetStyle();
        style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
        style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        // Dimensions
        ImGuiIO &io = ImGui::GetIO();
        io.FontGlobalScale = scale;
    }

    void UIOverlay::initialize(const RenderPassInitInfo *info) {
        PassBase::initialize(info);
        const auto *_info = static_cast<const UIPassInitInfo *>(info);
        queue = _info->queue;
        prepareResources();
        preparePipeline(_info->renderPass, _info->colorFormat, _info->depthFormat);
    }

    void UIOverlay::clean(){
        freeResources();
    }

    UIOverlay::~UIOverlay() {
        if (ImGui::GetCurrentContext()) {
            ImGui::DestroyContext();
        }
    }

    /** Prepare all vulkan resources required to render the UI overlay */
    void UIOverlay::prepareResources() {
        ImGuiIO &io = ImGui::GetIO();

        // Create font texture
        unsigned char *fontData;
        int texWidth, texHeight;
        const std::string filename = getAssetPath() + "Roboto-Medium.ttf";
        io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * scale);
        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
        VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

        //SRS - Set ImGui style scale factor to handle retina and other HiDPI displays (same as font scaling above)
        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(scale);

        // Create target image for copy
        VkImageCreateInfo imageInfo = CreateImageCreateInfo();
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.extent.width = texWidth;
        imageInfo.extent.height = texHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vkCreateImage(device->device, &imageInfo, nullptr, &fontImage);
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device->device, fontImage, &memReqs);
//        VkMemoryAllocateInfo memAllocInfo = MW::initializers::memoryAllocateInfo();
        VkMemoryAllocateInfo memAllocInfo{};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = device->findMemoryType(memReqs.memoryTypeBits,
                                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(device->device, &memAllocInfo, nullptr, &fontMemory);
        vkBindImageMemory(device->device, fontImage, fontMemory, 0);

        // Image view
        VkImageViewCreateInfo viewInfo = CreateImageViewCreateInfo();
        viewInfo.image = fontImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device->device, &viewInfo, nullptr, &fontView);

        // Staging buffers for font data upload
        VulkanBuffer stagingBuffer;
        device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, uploadSize);

        device->MapMemory(stagingBuffer);
        memcpy(stagingBuffer.mapped, fontData, uploadSize);
        device->unMapMemory(stagingBuffer);

        // Copy buffer data to font image
        VkCommandBuffer copyCmd = device->beginSingleTimeCommands();
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;
        // Prepare for transfer
        device->transitionImageLayout(
                copyCmd,
                fontImage,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                subresourceRange,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);

        // Copy
        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = texWidth;
        bufferCopyRegion.imageExtent.height = texHeight;
        bufferCopyRegion.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(
                copyCmd,
                stagingBuffer.buffer,
                fontImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &bufferCopyRegion
        );
        // Prepare for shader read
        device->transitionImageLayout(
                copyCmd,
                fontImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                subresourceRange,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        device->endSingleTimeCommands(copyCmd);

        device->DestroyVulkanBuffer(stagingBuffer);

        // Font texture Sampler
        VkSamplerCreateInfo samplerInfo = CreateSamplerCreateInfo();
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        device->CreateSampler(&samplerInfo, &sampler);
        // Descriptor pool
//        std::vector<VkDescriptorPoolSize> poolSizes = {
//                CreateDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
//        };
//        VkDescriptorPoolCreateInfo descriptorPoolInfo = CreateDescriptorPoolCreateInfo(poolSizes, 2);
//        vkCreateDescriptorPool(device->device, &descriptorPoolInfo, nullptr, &descriptorPool);

        // Descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo descriptorLayout = CreateDescriptorSetLayoutCreateInfo(
                setLayoutBindings);
        vkCreateDescriptorSetLayout(device->device, &descriptorLayout, nullptr, &descriptorSetLayout);

        // Descriptor set
        device->CreateDescriptorSet(1, descriptorSetLayout, descriptorSet);
//        VkDescriptorSetAllocateInfo allocInfo = CreateDescriptorSetAllocateInfo(descriptorPool,
//                                                                                            &descriptorSetLayout, 1);
//        vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
        VkDescriptorImageInfo fontDescriptor = CreateDescriptorImageInfo(
                sampler,
                fontView,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                CreateWriteDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                         &fontDescriptor)
        };
        vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(writeDescriptorSets.size()),
                               writeDescriptorSets.data(), 0, nullptr);
    }

    /** Prepare a separate pipeline for the UI overlay rendering decoupled from the main application */
    void UIOverlay::preparePipeline(const VkRenderPass renderPass,
                                    const VkFormat colorFormat, const VkFormat depthFormat) {
        auto vertShaderModule = device->CreateShaderModule(UIOVERLAY_VERT);
        auto fragShaderModule = device->CreateShaderModule(UIOVERLAY_FRAG);
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        shaders = {vertShaderStageInfo, fragShaderStageInfo};
        // Pipeline layout
        // Push constants for UI rendering parameters
        VkPushConstantRange pushConstantRange = CreatePushConstantRange(VK_SHADER_STAGE_VERTEX_BIT,
                                                                        sizeof(PushConstBlock), 0);
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = CreatePipelineLayoutCreateInfo(
                &descriptorSetLayout, 1);
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        device->CreatePipelineLayout(&pipelineLayoutCreateInfo, &pipelineLayout);
        // Setup graphics pipeline for UI rendering
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
                CreatePipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0,
                                                           VK_FALSE);

        VkPipelineRasterizationStateCreateInfo rasterizationState =
                CreatePipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                                           VK_FRONT_FACE_COUNTER_CLOCKWISE);

        // Enable blending
        VkPipelineColorBlendAttachmentState blendAttachmentState{};
        blendAttachmentState.blendEnable = VK_TRUE;
        blendAttachmentState.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
        blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendState =
                CreatePipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

        VkPipelineDepthStencilStateCreateInfo depthStencilState =
                CreatePipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

        VkPipelineViewportStateCreateInfo viewportState =
                CreatePipelineViewportStateCreateInfo(1, 1, 0);

        VkPipelineMultisampleStateCreateInfo multisampleState =
                CreatePipelineMultisampleStateCreateInfo(rasterizationSamples);

        std::vector<VkDynamicState> dynamicStateEnables = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState =
                CreatePipelineDynamicStateCreateInfo(dynamicStateEnables);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = CreatePipelineCreateInfo(pipelineLayout,
                                                                                   renderPass);

        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.pMultisampleState = &multisampleState;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pDepthStencilState = &depthStencilState;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaders.size());
        pipelineCreateInfo.pStages = shaders.data();
        pipelineCreateInfo.subpass = subpass;

#if defined(VK_KHR_dynamic_rendering)
        // SRS - if we are using dynamic rendering (i.e. renderPass null), must define color, depth and stencil attachments at pipeline create time
        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {};
        if (renderPass == VK_NULL_HANDLE) {
            pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
            pipelineRenderingCreateInfo.colorAttachmentCount = 1;
            pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
            pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
            pipelineRenderingCreateInfo.stencilAttachmentFormat = depthFormat;
            pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
        }
#endif

        // Vertex bindings an attributes based on ImGui vertex definition
        std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
                CreateVertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
        };
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
                CreateVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT,
                                                      offsetof(ImDrawVert, pos)),    // Location 0: Position
                CreateVertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT,
                                                      offsetof(ImDrawVert, uv)),    // Location 1: UV
                CreateVertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM,
                                                      offsetof(ImDrawVert, col)),    // Location 0: Color
        };
        VkPipelineVertexInputStateCreateInfo vertexInputState = CreatePipelineVertexInputStateCreateInfo();
        vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
        vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
        vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
        vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

        pipelineCreateInfo.pVertexInputState = &vertexInputState;
        device->CreateGraphicsPipelines(&pipelineCreateInfo, &pipeline);
//        vkCreateGraphicsPipelines(device->device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline);
        device->DestroyShaderModule(vertShaderModule);
        device->DestroyShaderModule(fragShaderModule);
    }

    /** Update vertex and index buffer containing the imGui elements when required */
    bool UIOverlay::update() {
        ImDrawData *imDrawData = ImGui::GetDrawData();
        bool updateCmdBuffers = false;

        if (!imDrawData) { return false; };

        // Note: Alignment is done inside buffer creation
        VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
        VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

        // Update buffers only if vertex or index count has been changed compared to current buffer size
        if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
            return false;
        }

        // Vertex buffer
        if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
            device->unMapMemory(vertexBuffer);
            engineGlobalContext.renderSystem->registerGarbageBuffer(vertexBuffer);
//            device->DestroyVulkanBuffer(vertexBuffer);
            device->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                 vertexBuffer, vertexBufferSize);
            vertexCount = imDrawData->TotalVtxCount;
            device->MapMemory(vertexBuffer);
            updateCmdBuffers = true;
        }

        // Index buffer
        if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {
            device->unMapMemory(indexBuffer);
            engineGlobalContext.renderSystem->registerGarbageBuffer(indexBuffer);
//            device->DestroyVulkanBuffer(indexBuffer);
            device->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indexBuffer,
                                 indexBufferSize);
            indexCount = imDrawData->TotalIdxCount;
            device->MapMemory(indexBuffer);
            updateCmdBuffers = true;
        }

        // Upload data
        ImDrawVert *vtxDst = (ImDrawVert *) vertexBuffer.mapped;
        ImDrawIdx *idxDst = (ImDrawIdx *) indexBuffer.mapped;

        for (int n = 0; n < imDrawData->CmdListsCount; n++) {
            const ImDrawList *cmd_list = imDrawData->CmdLists[n];
            memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmd_list->VtxBuffer.Size;
            idxDst += cmd_list->IdxBuffer.Size;
        }

        // Flush to make writes visible to GPU
        device->flushBuffer(vertexBuffer);
        device->flushBuffer(indexBuffer);
        visible = true;
        return updateCmdBuffers;
    }

    void UIOverlay::draw(const VkCommandBuffer commandBuffer) {
        if (!visible)return;
        ImDrawData *imDrawData = ImGui::GetDrawData();
        int32_t vertexOffset = 0;
        int32_t indexOffset = 0;

        if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
            return;
        }

        ImGuiIO &io = ImGui::GetIO();

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                                NULL);

        pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
        pushConstBlock.translate = glm::vec2(-1.0f);
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock),
                           &pushConstBlock);

        VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

        for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
            const ImDrawList *cmd_list = imDrawData->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
                const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
                VkRect2D scissorRect;
                scissorRect.offset.x = std::max((int32_t) (pcmd->ClipRect.x), 0);
                scissorRect.offset.y = std::max((int32_t) (pcmd->ClipRect.y), 0);
                scissorRect.extent.width = (uint32_t) (pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissorRect.extent.height = (uint32_t) (pcmd->ClipRect.w - pcmd->ClipRect.y);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
                vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
    }

    void UIOverlay::resize(uint32_t width, uint32_t height) {
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float) (width), (float) (height));
    }

    void UIOverlay::freeResources() {
        device->DestroyVulkanBuffer(vertexBuffer);
        device->DestroyVulkanBuffer(indexBuffer);
        vkDestroyImageView(device->device, fontView, nullptr);
        vkDestroyImage(device->device, fontImage, nullptr);
        vkFreeMemory(device->device, fontMemory, nullptr);
        vkDestroySampler(device->device, sampler, nullptr);
        vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);
        vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
        vkDestroyPipeline(device->device, pipeline, nullptr);
    }

    bool UIOverlay::header(const char *caption) {
        return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
    }

    bool UIOverlay::checkBox(const char *caption, bool *value) {
        bool res = ImGui::Checkbox(caption, value);
        if (res) { updated = true; };
        return res;
    }

    bool UIOverlay::checkBox(const char *caption, int32_t *value) {
        bool val = (*value == 1);
        bool res = ImGui::Checkbox(caption, &val);
        *value = val;
        if (res) { updated = true; };
        return res;
    }

    bool UIOverlay::radioButton(const char *caption, bool value) {
        bool res = ImGui::RadioButton(caption, value);
        if (res) { updated = true; };
        return res;
    }

    bool UIOverlay::inputFloat(const char *label, float *v, float step, float step_fast, const char *format,
                               ImGuiInputTextFlags flags) {
        bool res = ImGui::InputFloat(label, v, step, step_fast, format, flags);
        if (res) { updated = true; };
        return res;
    }

    bool UIOverlay::sliderFloat(const char *caption, float *value, float min, float max) {
        bool res = ImGui::SliderFloat(caption, value, min, max);
        if (res) { updated = true; };
        return res;
    }

    bool UIOverlay::sliderInt(const char *caption, int32_t *value, int32_t min, int32_t max) {
        bool res = ImGui::SliderInt(caption, value, min, max);
        if (res) { updated = true; };
        return res;
    }

    bool UIOverlay::comboBox(const char *caption, int32_t *itemindex, std::vector<std::string> items) {
        if (items.empty()) {
            return false;
        }
        std::vector<const char *> charitems;
        charitems.reserve(items.size());
        for (size_t i = 0; i < items.size(); i++) {
            charitems.push_back(items[i].c_str());
        }
        uint32_t itemCount = static_cast<uint32_t>(charitems.size());
        bool res = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
        if (res) { updated = true; };
        return res;
    }

    bool UIOverlay::button(const char *caption) {
        bool res = ImGui::Button(caption);
        if (res) { updated = true; };
        return res;
    }

    bool UIOverlay::colorPicker(const char *caption, float *color) {
        bool res = ImGui::ColorEdit4(caption, color, ImGuiColorEditFlags_NoInputs);
        if (res) { updated = true; };
        return res;
    }

    void UIOverlay::text(const char *formatstr, ...) {
        va_list args;
                va_start(args, formatstr);
        ImGui::TextV(formatstr, args);
                va_end(args);
    }
}
