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

namespace MW {
    class WindowSystem;


    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> computeFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();;
        }
    };

    struct VulkanDeviceInitInfo {
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
        VulkanDevice(const VulkanDevice &) = delete;

        VulkanDevice &operator=(const VulkanDevice &) = delete;

        VulkanDevice(VulkanDevice &&) = delete;

        VulkanDevice &operator=(VulkanDevice &&) = delete;

        VkCommandPool getCommandPool() { return commandPool; }

        VkDevice device() { return device_; }

        VkSurfaceKHR surface() { return surface_; }

        VkQueue graphicsQueue() { return graphicsQueue_; }

        VkQueue presentQueue() { return presentQueue_; }

        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }

        VkFormat findSupportedFormat(
                const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        // Buffer Helper Functions
        void CreateBuffer(
                VkDeviceSize size,
                VkBufferUsageFlags usage,
                VkMemoryPropertyFlags properties,
                VkBuffer &buffer,
                VkDeviceMemory &bufferMemory);

        void CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                       VkDescriptorSetLayout *pSetLayout,
                                       const VkAllocationCallbacks *pAllocator = nullptr);

        void copyBuffer(VkBuffer *srcBuffer, VkBuffer *dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset,
                        VkDeviceSize size);

        void CreateImage(uint32_t image_width, uint32_t image_height, VkFormat format, VkImageTiling image_tiling,
                         VkImageUsageFlags image_usage_flags, VkMemoryPropertyFlags memory_property_flags,
                         VkImage *image, VkDeviceMemory *memory, VkImageCreateFlags image_Create_flags,
                         uint32_t array_layers, uint32_t miplevels);

        void CreateImageView(VkImage *image, VkFormat format, VkImageAspectFlags image_aspect_flags,
                             VkImageViewType view_type, uint32_t layout_count, uint32_t miplevels,
                             VkImageView *image_view);

        void CreateGlobalImage(VkImage *image, VkImageView *image_view, VmaAllocation &image_allocation,
                               uint32_t texture_image_width, uint32_t texture_image_height, void *texture_image_pixels,
                               VkFormat texture_image_format, uint32_t miplevels = 0);

        void CreateCubeMap(VkImage *image, VkImageView *image_view, VmaAllocation &image_allocation,
                           uint32_t texture_image_width, uint32_t texture_image_height,
                           std::array<void *, 6> texture_image_pixels, VkFormat texture_image_format,
                           uint32_t miplevels);

        void CreateCommandPool(const VkCommandPoolCreateInfo *pCreateInfo, VkCommandPool *pCommandPool,
                               const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateDescriptorPool(const VkDescriptorPoolCreateInfo *pCreateInfo, VkDescriptorPool *pDescriptorPool,
                                  const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                       VkDescriptorSetLayout *pSetLayout,
                                       const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateFramebuffer(const VkFramebufferCreateInfo *pCreateInfo, VkFramebuffer *pFramebuffer,
                               const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t CreateInfoCount,
                                     const VkGraphicsPipelineCreateInfo *pCreateInfos, VkPipeline *pPipelines,
                                     const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateComputePipelines(VkPipelineCache pipelineCache, uint32_t CreateInfoCount,
                                    const VkComputePipelineCreateInfo *pCreateInfos, VkPipeline *pPipelines,
                                    const VkAllocationCallbacks *pAllocator = nullptr);

        void CreatePipelineLayout(const VkPipelineLayoutCreateInfo *pCreateInfo, VkPipelineLayout *pPipelineLayout,
                                  const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateRenderPass(const VkRenderPassCreateInfo *pCreateInfo, VkRenderPass *pRenderPass,
                              const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateSampler(const VkSamplerCreateInfo *pCreateInfo, VkSampler *pSampler,
                           const VkAllocationCallbacks *pAllocator = nullptr);
         void  AllocateDescriptorSets(
                const VkDescriptorSetAllocateInfo*          pAllocateInfo,
                VkDescriptorSet*                            pDescriptorSets);
        VkCommandBuffer beginSingleTimeCommands();

        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        void copyBufferToImage(
                VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

        void CreateImageWithInfo(const VkImageCreateInfo &imageInfo,
                                 VkMemoryPropertyFlags properties,
                                 VkImage &image,
                                 VkDeviceMemory &imageMemory);

        VkResult AcquireNextImageKHR(VkSwapchainKHR swapchain,
                                     uint64_t timeout,
                                     VkSemaphore semaphore,
                                     VkFence fence,
                                     uint32_t *pImageIndex);

        VkPhysicalDeviceProperties properties;


        void updateDescriptorSets(const VkWriteDescriptorSet *pDescriptorWrites,
                                  uint32_t descriptorWriteCount = 1,
                                  const VkCopyDescriptorSet *pDescriptorCopies = nullptr,
                                  uint32_t descriptorCopyCount = 0);

        void QueueSubmit(VkQueue queue,
                         uint32_t submitCount,
                         const VkSubmitInfo *pSubmits,
                         VkFence fence);

        void CreateSwapchainKHR(const VkSwapchainCreateInfoKHR *pCreateInfo,
                                VkSwapchainKHR *pSwapchain,
                                const VkAllocationCallbacks *pAllocator = nullptr);

        void GetSwapchainImagesKHR(VkSwapchainKHR swapchain,
                                   uint32_t *pSwapchainImageCount,
                                   VkImage *pSwapchainImages);

        void CreateSemaphore(const VkSemaphoreCreateInfo *pCreateInfo,
                             VkSemaphore *pSemaphore,
                             const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateFence(const VkFenceCreateInfo *pCreateInfo,
                         VkFence *pFence,
                         const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateImageView(const VkImageViewCreateInfo *pCreateInfo,
                             VkImageView *pView,
                             const VkAllocationCallbacks *pAllocator = nullptr);

        void ResetFences(uint32_t fenceCount, const VkFence *pFences);

        void CreateDescriptorPool();

        void WaitForFences(uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll = VK_TRUE,
                           uint64_t timeout = std::numeric_limits<uint64_t>::max());

        void DestroyImageView(VkImageView imageView, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroyImage(VkImage image, const VkAllocationCallbacks *pAllocator = nullptr);

        void FreeMemory(VkDeviceMemory deviceMemory, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroySemaphore(VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroyFence(VkFence fence, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroySwapchainKHR(VkSwapchainKHR swapChain, const VkAllocationCallbacks *pAllocator = nullptr);

        VkImageView getImageView(int index) { return swapChainImageViews[index]; }

        VulkanSwapChainDesc getSwapchainInfo();

        VulkanDepthImageDesc getDepthImageInfo() const;

        size_t imageCount() { return swapChainImages.size(); }

        VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }

        VkExtent2D getSwapChainExtent() { return swapChainExtent; }

        uint32_t width() { return swapChainExtent.width; }

        uint32_t height() { return swapChainExtent.height; }

        VkDescriptorPool getDescriptorPool() { return descriptorPool; }

        float extentAspectRatio() {
            return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        }

        VkFormat findDepthFormat();

        VkResult acquireNextImage(uint32_t *imageIndex);

        VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        VkFormat swapChainImageFormat;
        VkFormat swapChainDepthFormat;
        VkExtent2D swapChainExtent;
        VkViewport swapChainViewport;
        VkRect2D swapChainScissor;

        VkImage depthImages;
        VkDeviceMemory depthImageMemorys;
        VkImageView depthImageViews;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;

        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        size_t currentFrame = 0;
    private:
        void CreateInstance();

        void setupDebugMessenger();

        void CreateSurface();

        void pickPhysicalDevice();

        void CreateLogicalDevice();

        void CreateCommandPool();

        // helper functions
        bool isDeviceSuitable(VkPhysicalDevice device);

        std::vector<const char *> getRequiredExtensions();

        bool checkValidationLayerSupport();

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &CreateInfo);

        void hasGflwRequiredInstanceExtensions();

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        /*WindowSystem& window;*/
        GLFWwindow *window{nullptr};
        VkCommandPool commandPool;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue graphicsQueue_;
        VkQueue presentQueue_;
        VkDescriptorPool descriptorPool;

        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};


        void createSwapChain();

        void createImageViews();

        void createDepthResources();

        void createRenderPass();

        void createFramebuffers();

        void createSyncObjects();

        // Helper functions
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(
                const std::vector<VkSurfaceFormatKHR> &availableFormats);

        VkPresentModeKHR chooseSwapPresentMode(
                const std::vector<VkPresentModeKHR> &availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
    };

}