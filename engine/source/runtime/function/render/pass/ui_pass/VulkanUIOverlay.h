/*
* UI overlay class using ImGui
*
* Copyright (C) 2017-2024 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include <vulkan/vulkan.h>
#include "runtime/function/render/pass/pass_base/pass_base.h"
#include "function/render/rhi/vulkan_device.h"
#include "function/render/rhi/vulkan_resources.h"
#include "function/render/rhi/vulkan_util.h"
#include "imgui.h"
#include "glm/glm.hpp"

namespace MW
{
    struct UIPassInitInfo : public RenderPassInitInfo {
        VkRenderPass renderPass;
        VkFormat colorFormat;
        VkFormat depthFormat;
        VkQueue queue;
        explicit UIPassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };
    class UIOverlay : public PassBase
	{
	public:
//		VulkanDevice* device{ nullptr };
		VkQueue queue{ VK_NULL_HANDLE };

		VkSampleCountFlagBits rasterizationSamples{ VK_SAMPLE_COUNT_1_BIT };
		uint32_t subpass{ main_camera_subpass_ui_pass };

        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;
		int32_t vertexCount{ 0 };
		int32_t indexCount{ 0 };

		std::vector<VkPipelineShaderStageCreateInfo> shaders;

		VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
		VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
		VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
		VkPipeline pipeline{ VK_NULL_HANDLE };

		VkDeviceMemory fontMemory{ VK_NULL_HANDLE };
		VkImage fontImage{ VK_NULL_HANDLE };
		VkImageView fontView{ VK_NULL_HANDLE };
		VkSampler sampler{ VK_NULL_HANDLE };

		struct PushConstBlock {
			glm::vec2 scale;
			glm::vec2 translate;
		} pushConstBlock;

		bool visible{ false };
		bool updated{ false };
		float scale{ 1.0f };
		float updateTimer{ 0.0f };

		UIOverlay();
		~UIOverlay();
        void initialize(const RenderPassInitInfo *info) override;
        void clean() override;

		void preparePipeline(const VkRenderPass renderPass, const VkFormat colorFormat, const VkFormat depthFormat);
		void prepareResources();

		bool update();
		void draw(const VkCommandBuffer commandBuffer);
		void resize(uint32_t width, uint32_t height);

		void freeResources();

		bool header(const char* caption);
		bool checkBox(const char* caption, bool* value);
		bool checkBox(const char* caption, int32_t* value);
		bool radioButton(const char* caption, bool value);
        bool inputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0);
//        bool inputFloat(const char* caption, float* value, float step, uint32_t precision);
		bool sliderFloat(const char* caption, float* value, float min, float max);
		bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
		bool comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
		bool button(const char* caption);
		bool colorPicker(const char* caption, float* color);
		void text(const char* formatstr, ...);
	};
}
