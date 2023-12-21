#pragma once
#define NDEBUG
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <optional>
#include "vulkan_resources.h"

namespace MW
{
    class WindowSystem;


    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> computeFamily;

        bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();; }
    };
    struct VulkanDeviceInitInfo
    {
        std::shared_ptr<WindowSystem> windowSystem;
    };
    class VulkanDevice {
    public:
#ifdef NDEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif

        VulkanDevice(VulkanDeviceInitInfo info);

        ~VulkanDevice();

        // Not copyable or movable
        VulkanDevice(const VulkanDevice&) = delete;

        VulkanDevice& operator=(const VulkanDevice&) = delete;

        VulkanDevice(VulkanDevice&&) = delete;

        VulkanDevice& operator=(VulkanDevice&&) = delete;

        VkCommandPool getCommandPool() { return commandPool; }

        VkDevice device() { return device_; }

        VkSurfaceKHR surface() { return surface_; }

        VkQueue graphicsQueue() { return graphicsQueue_; }

        VkQueue presentQueue() { return presentQueue_; }

        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }

        VkFormat findSupportedFormat(
            const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        // Buffer Helper Functions
        void createBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory);
        void copyBuffer(VkBuffer* srcBuffer, VkBuffer* dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size) ;
        void createImage(uint32_t image_width, uint32_t image_height, VkFormat format, VkImageTiling image_tiling, VkImageUsageFlags image_usage_flags, VkMemoryPropertyFlags memory_property_flags,
            VkImage*& image, VkDeviceMemory*& memory, VkImageCreateFlags image_create_flags, uint32_t array_layers, uint32_t miplevels) ;
        void createImageView(VkImage* image, VkFormat format, VkImageAspectFlags image_aspect_flags, VkImageViewType view_type, uint32_t layout_count, uint32_t miplevels,
            VkImageView*& image_view) ;
        void createGlobalImage(VkImage*& image, VkImageView*& image_view, VmaAllocation& image_allocation, uint32_t texture_image_width, uint32_t texture_image_height, void* texture_image_pixels, VkFormat texture_image_format, uint32_t miplevels = 0) ;
        void createCubeMap(VkImage*& image, VkImageView*& image_view, VmaAllocation& image_allocation, uint32_t texture_image_width, uint32_t texture_image_height, std::array<void*, 6> texture_image_pixels, VkFormat texture_image_format, uint32_t miplevels) ;
        bool createCommandPool(const VkCommandPoolCreateInfo* pCreateInfo, VkCommandPool*& pCommandPool) ;
        bool createDescriptorPool(const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool*& pDescriptorPool) ;
        bool createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout*& pSetLayout) ;
        bool createFence(const VkFenceCreateInfo* pCreateInfo, VkFence*& pFence) ;
        bool createFramebuffer(const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer*& pFramebuffer) ;
        bool createGraphicsPipelines(VkPipelineCache* pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline*& pPipelines) ;
        bool createComputePipelines(VkPipelineCache* pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, VkPipeline*& pPipelines) ;
        bool createPipelineLayout(const VkPipelineLayoutCreateInfo* pCreateInfo, VkPipelineLayout*& pPipelineLayout) ;
        bool createRenderPass(const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass*& pRenderPass) ;
        bool createSampler(const VkSamplerCreateInfo* pCreateInfo, VkSampler*& pSampler) ;
        bool createSemaphore(const VkSemaphoreCreateInfo* pCreateInfo, VkSemaphore*& pSemaphore) ;

        VkCommandBuffer beginSingleTimeCommands();

        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        void copyBufferToImage(
            VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

        void createImageWithInfo(
            const VkImageCreateInfo& imageInfo,
            VkMemoryPropertyFlags properties,
            VkImage& image,
            VkDeviceMemory& imageMemory);
        VkResult AcquireNextImageKHR(
            VkSwapchainKHR swapchain,
            uint64_t timeout,
            VkSemaphore semaphore,
            VkFence fence,
            uint32_t* pImageIndex);
        VkPhysicalDeviceProperties properties;
        void QueueSubmit(
            VkQueue queue,
            uint32_t submitCount,
            const VkSubmitInfo* pSubmits,
            VkFence fence);
        void CreateSwapchainKHR(
            const VkSwapchainCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSwapchainKHR* pSwapchain);
        void GetSwapchainImagesKHR(
            VkSwapchainKHR swapchain,
            uint32_t* pSwapchainImageCount,
            VkImage* pSwapchainImages);
        void ResetFences(uint32_t fenceCount,const VkFence* pFences);
        void WaitForFences(uint32_t fenceCount,const VkFence* pFences,VkBool32 waitAll = VK_TRUE,uint64_t timeout= std::numeric_limits<uint64_t>::max());
        void DestroyImageView(VkImageView imageView, const VkAllocationCallbacks* pAllocator = nullptr);
        void DestroyImage(VkImage image, const VkAllocationCallbacks* pAllocator = nullptr);
        void FreeMemory(VkDeviceMemory deviceMemory, const VkAllocationCallbacks* pAllocator = nullptr);
        void DestroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator = nullptr);
        void DestroySemaphore(VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator = nullptr);
        void DestroyFence(VkFence fence, const VkAllocationCallbacks* pAllocator = nullptr);
        void DestroySwapchainKHR(VkSwapchainKHR swapChain, const VkAllocationCallbacks* pAllocator = nullptr);
    private:
        void createInstance();

        void setupDebugMessenger();

        void createSurface();

        void pickPhysicalDevice();

        void createLogicalDevice();

        void createCommandPool();

        // helper functions
        bool isDeviceSuitable(VkPhysicalDevice device);

        std::vector<const char*> getRequiredExtensions();

        bool checkValidationLayerSupport();

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

        void hasGflwRequiredInstanceExtensions();

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        /*WindowSystem& window;*/
        GLFWwindow* window{ nullptr };
        VkCommandPool commandPool;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue graphicsQueue_;
        VkQueue presentQueue_;

        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    };

}