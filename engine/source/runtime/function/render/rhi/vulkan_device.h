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
#include "vulkan_util.h"

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

        VulkanDevice();

        ~VulkanDevice();

        // Not copyable or movable
        VulkanDevice(const VulkanDevice &) = delete;

        VulkanDevice &operator=(const VulkanDevice &) = delete;

        VulkanDevice(VulkanDevice &&) = delete;

        VulkanDevice &operator=(VulkanDevice &&) = delete;

        void initialize(VulkanDeviceInitInfo info);

        void clean();

        VkCommandPool getCommandPool() { return commandPool; }

        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }

        VkFormat findSupportedFormat(
                const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        void findFunctionRequired();

        // Buffer Helper Functions
        void CreateBuffer(
                VkDeviceSize size,
                VkBufferUsageFlags usage,
                VkMemoryPropertyFlags properties,
                VkBuffer &buffer,
                VkDeviceMemory &bufferMemory,
                const void *pNext = nullptr);

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

        void CreateDescriptorSet(uint32_t descriptorSetCount, const VkDescriptorSetLayout &pSetLayouts,
                                 VkDescriptorSet &set);

        void AllocateDescriptorSets(const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                    VkDescriptorSet *pDescriptorSets);

        void UpdateDescriptorSets(uint32_t descriptorWriteCount,
                                  const VkWriteDescriptorSet *pDescriptorWrites,
                                  uint32_t descriptorCopyCount = 0,
                                  const VkCopyDescriptorSet *pDescriptorCopies = nullptr);

        void CreateFramebuffer(const VkFramebufferCreateInfo *pCreateInfo, VkFramebuffer *pFramebuffer,
                               const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t CreateInfoCount,
                                     const VkGraphicsPipelineCreateInfo *pCreateInfos, VkPipeline *pPipelines,
                                     const VkAllocationCallbacks *pAllocator = nullptr);

        void CreateGraphicsPipelines(const VkGraphicsPipelineCreateInfo *pCreateInfos, VkPipeline *pPipelines);

        void CreateRayTracingPipelinesKHR(const VkRayTracingPipelineCreateInfoKHR *pCreateInfos,
                                          VkPipeline *pPipelines,
                                          uint32_t createInfoCount = 1,
                                          VkDeferredOperationKHR deferredOperation = VK_NULL_HANDLE,
                                          VkPipelineCache pipelineCache = VK_NULL_HANDLE,
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

        VkCommandBuffer beginSingleTimeCommands();

        VkShaderModule CreateShaderModule(const std::vector<unsigned char> &shader_code);

        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        void copyBufferToImage(
                VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

        void CreateImageWithInfo(const VkImageCreateInfo &imageInfo,
                                 VkMemoryPropertyFlags propertiesFlags,
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

        void
        CreateDeviceBuffer(VkBufferUsageFlags usageFlags, VulkanBuffer &buffer,
                           VkDeviceSize size, void *data = nullptr);

        void
        CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VulkanBuffer &buffer,
                     VkDeviceSize size, void *data = nullptr);

        void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                   bool useCurrentCommandBuffer = false,
                                   VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
                                   VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        void transitionImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldLayout,
                                   VkImageLayout newLayout,
                                   VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
                                   VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        void ResetFences(uint32_t fenceCount, const VkFence *pFences);

        void CreateDescriptorPool();

        void WaitForFences(uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll = VK_TRUE,
                           uint64_t timeout = std::numeric_limits<uint64_t>::max());

        void DestroyImageView(VkImageView imageView, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroyImage(VkImage image, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroySampler(VkSampler sampler, const VkAllocationCallbacks *pAllocator = nullptr);

        void FreeMemory(VkDeviceMemory deviceMemory, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroySemaphore(VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroyFence(VkFence fence, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroySwapchainKHR(VkSwapchainKHR swapChain, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroyShaderModule(VkShaderModule module, const VkAllocationCallbacks *pAllocator = nullptr);

        void DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout,
                                        const VkAllocationCallbacks *pAllocator = nullptr);

        void recreateSwapChain();

        bool prepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapchain);

        void MapMemory(VulkanBuffer &vulkanBuffer, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void flushBuffer(VulkanBuffer &buffer, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void unMapMemory(VulkanBuffer &buffer);

        void getEnabledFeatures();

        VkImageView getImageView(int index) { return swapChainImageViews[index]; }

        VulkanSwapChainDesc getSwapchainInfo();

        VulkanDepthImageDesc getDepthImageInfo() const;

        size_t imageCount() { return swapChainImages.size(); }

        VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }

        VkExtent2D getSwapChainExtent() { return swapChainExtent; }

        uint32_t width() { return swapChainExtent.width; }

        uint32_t height() { return swapChainExtent.height; }

        VkImage getCurrentImage() { return swapChainImages[currentImageIndex]; }

        VkDescriptorPool getDescriptorPool() { return descriptorPool; }

        void GetPhysicalDeviceProperties2(VkPhysicalDeviceProperties2 *pProperties);

        void GetPhysicalDeviceFeatures2(VkPhysicalDeviceFeatures2 *pFeatures);

        void GetPhysicalDeviceFormatProperties(VkFormat format, VkFormatProperties *pFormatProperties);

        void createAccelerationStructureBuffer(AccelerationStructure &accelerationStructure,
                                               VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

        VkCommandBuffer getCurrentCommandBuffer() const {
            return commandBuffers[currentFrameIndex];
        }

        int getFrameIndex() const {
            return currentFrameIndex;
        }

        float extentAspectRatio() {
            return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        }

        void waitForFences();

        void resetCommandPool();

        VkFormat findDepthFormat();

        VkResult acquireNextImage(uint32_t *imageIndex);

        void submitCommandBuffers(std::function<void()> passUpdateAfterRecreateSwapchain);

        uint64_t getBufferDeviceAddress(VkBuffer &buffer);

        void GetAccelerationStructureBuildSizesKHR(
                VkAccelerationStructureBuildTypeKHR buildType,
                const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo,
                const uint32_t *pMaxPrimitiveCounts,
                VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo);

        void CreateAccelerationStructureKHR(
                const VkAccelerationStructureCreateInfoKHR *pCreateInfo,
                VkAccelerationStructureKHR *pAccelerationStructure,
                const VkAllocationCallbacks *pAllocator = nullptr);

        RayTracingScratchBuffer createScratchBuffer(VkDeviceSize size);

        void destroyScratchBuffer(RayTracingScratchBuffer &scratchBuffer);

        void destroyVulkanBuffer(VulkanBuffer &vulkanBuffer);

        uint32_t getAccelerationStructureDeviceAddressKHR(const VkAccelerationStructureDeviceAddressInfoKHR *pInfo);

        void GetRayTracingShaderGroupHandlesKHR(VkPipeline pipeline,
                                                uint32_t firstGroup,
                                                uint32_t groupCount,
                                                size_t dataSize,
                                                void *pData);

        void CmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer,
                                               uint32_t infoCount,
                                               const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
                                               const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos);

        void CmdTraceRaysKHR(VkCommandBuffer commandBuffer,
                             const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable,
                             const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable,
                             const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable,
                             const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable,
                             uint32_t width,
                             uint32_t height,
                             uint32_t depth);

        void BuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer,
                                            uint32_t infoCount,
                                            const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
                                            const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos);

        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        VkFormat swapChainImageFormat;
        VkFormat swapChainDepthFormat;
        VkExtent2D swapChainExtent;
        VkViewport swapChainViewport;
        VkRect2D swapChainScissor;

        VkImage depthImages = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemorys;
        VkImageView depthImageViews = VK_NULL_HANDLE;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;

        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;
        VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
        VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
        VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
        VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
        uint32_t currentImageIndex{};
        int currentFrameIndex{};
        QueueFamilyIndices queueIndices;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

//    private:
        void CreateInstance();

        void setupDebugMessenger();

        void CreateSurface();

        void pickPhysicalDevice();

        void CreateLogicalDevice();

        void CreateCommandPool();

        void CreateCommandBuffers();

        // helper functions
        bool isDeviceSuitable(VkPhysicalDevice device);

        std::vector<const char *> getRequiredExtensions();

        bool checkValidationLayerSupport();

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &CreateInfo);

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        /*WindowSystem& window;*/
        GLFWwindow *window{nullptr};
        VkCommandPool commandPool;
        VkCommandPool swapChainCommandPools[MAX_FRAMES_IN_FLIGHT];

        VkDevice device;
        VkSurfaceKHR surface;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkDescriptorPool descriptorPool;

        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                      VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME};
        uint32_t maxMaterialCount{256};

        void createSwapChain();

        void createSwapChainImageViews();

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

        PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
        PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
        PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
        PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
        PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
        PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
        PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
        PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
        PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
        PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

        VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
        void *deviceCreatepNextChain = nullptr;
        VkPhysicalDeviceFeatures enabledFeatures{};
    };

}