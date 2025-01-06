#include "vulkan_device.h"
#include "runtime/function/render/window_system.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
// std headers
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>
#include <array>

namespace MW {
    std::string errorString(VkResult errorCode) {
        switch (errorCode) {
#define STR(r) case VK_ ##r: return #r
            STR(NOT_READY);
            STR(TIMEOUT);
            STR(EVENT_SET);
            STR(EVENT_RESET);
            STR(INCOMPLETE);
            STR(ERROR_OUT_OF_HOST_MEMORY);
            STR(ERROR_OUT_OF_DEVICE_MEMORY);
            STR(ERROR_INITIALIZATION_FAILED);
            STR(ERROR_DEVICE_LOST);
            STR(ERROR_MEMORY_MAP_FAILED);
            STR(ERROR_LAYER_NOT_PRESENT);
            STR(ERROR_EXTENSION_NOT_PRESENT);
            STR(ERROR_FEATURE_NOT_PRESENT);
            STR(ERROR_INCOMPATIBLE_DRIVER);
            STR(ERROR_TOO_MANY_OBJECTS);
            STR(ERROR_FORMAT_NOT_SUPPORTED);
            STR(ERROR_SURFACE_LOST_KHR);
            STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
            STR(SUBOPTIMAL_KHR);
            STR(ERROR_OUT_OF_DATE_KHR);
            STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
            STR(ERROR_VALIDATION_FAILED_EXT);
            STR(ERROR_INVALID_SHADER_NV);
#undef STR
            default:
                return "UNKNOWN_ERROR";
        }
    }

#define VK_CHECK_RESULT(f)                                                                                \
{                                                                                                        \
    VkResult res = (f);                                                                                    \
    if (res != VK_SUCCESS)                                                                                \
    {                                                                                                    \
        std::cout << "Fatal : VkResult is \"" << errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
        assert(res == VK_SUCCESS);                                                                        \
    }                                                                                                    \
}

    // local callback functions
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    VkResult CreateDebugUtilsMessengerEXT(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
            const VkAllocationCallbacks *pAllocator,
            VkDebugUtilsMessengerEXT *pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
                instance,
                "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(
            VkInstance instance,
            VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks *pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
                instance,
                "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    // class member functions
    VulkanDevice::VulkanDevice() {
    }

    void VulkanDevice::initialize(VulkanDeviceInitInfo info) {
        window = info.windowSystem->getGLFWwindow();
        CreateInstance();
        setupDebugMessenger();
        CreateSurface();
        pickPhysicalDevice();
        getEnabledFeatures();
        CreateLogicalDevice();
        findFunctionRequired();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateDescriptorPool();
        createSwapChain();
        createSwapChainImageViews();
        createDepthResources();
#if USE_VRS
        createShadingRateImage();
#endif
        createSyncObjects();
    }

    void VulkanDevice::clean() {
        if(defaultNearestSampler != VK_NULL_HANDLE)
            DestroySampler(defaultNearestSampler);
        if(defaultLinearSampler != VK_NULL_HANDLE)
            DestroySampler(defaultLinearSampler);
        // cleanup synchronization objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            DestroySemaphore(renderFinishedSemaphores[i]);
            DestroySemaphore(imageAvailableSemaphores[i]);
            DestroyFence(inFlightFences[i]);
            // vkFreeCommandBuffers(device_,swapChainCommandPools[i],1,&commandBuffers[i]);
//            vkDestroyCommandPool(device, swapChainCommandPools[i], nullptr);
        }

        DestroyImageView(depthImageViews);
        DestroyImage(depthImages);
        FreeMemory(depthImageMemorys);
#if USE_VRS
        DestroyImageView(shadingRateImageViews);
        DestroyImage(shadingRateImages);
        FreeMemory(shadingRateImageMemorys);
#endif
        for (int i = 0; i < swapChainImages.size(); ++i) {
            DestroyImageView(swapChainImageViews[i]);
//            DestroyImage(swapChainImages[i]);
        }

        if (swapChain != nullptr) {
            DestroySwapchainKHR(swapChain);
        }
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkFreeCommandBuffers(device, swapChainCommandPools[i], 1, &commandBuffers[i]);
            vkDestroyCommandPool(device, swapChainCommandPools[i], nullptr);
        }
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    VulkanDevice::~VulkanDevice() {
    }

    void VulkanDevice::findFunctionRequired() {
        std::vector<void*>properties;
        properties.emplace_back(&rayTracingPipelineProperties);
        // Get ray tracing pipeline properties, which will be used later on in the sample
        rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
        VkPhysicalDeviceProperties2 deviceProperties2{};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &rayTracingPipelineProperties;
#if USE_VRS | 1
        // Get properties of this extensions, which also contains texel sizes required to setup the image
        physicalDeviceShadingRateImageProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
        rayTracingPipelineProperties.pNext = &physicalDeviceShadingRateImageProperties;
#endif
#if USE_MESH_SHADER | 1
        meshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
        physicalDeviceShadingRateImageProperties.pNext = &meshShaderProperties;
#endif
        GetPhysicalDeviceProperties2(&deviceProperties2);

        // Get acceleration structure properties, which will be used later on in the sample
        accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &accelerationStructureFeatures;
#if USE_MESH_SHADER | 1
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
        accelerationStructureFeatures.pNext = &meshShaderFeatures;
#endif
        GetPhysicalDeviceFeatures2(&deviceFeatures2);

        vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device,
                                                                                                            "vkGetBufferDeviceAddressKHR"));
        vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(
                device, "vkCmdBuildAccelerationStructuresKHR"));
        vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(
                device, "vkBuildAccelerationStructuresKHR"));
        vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(
                device, "vkCreateAccelerationStructureKHR"));
        vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(
                device, "vkDestroyAccelerationStructureKHR"));
        vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(
                device, "vkGetAccelerationStructureBuildSizesKHR"));
        vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(
                device, "vkGetAccelerationStructureDeviceAddressKHR"));
        vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
        vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(
                device, "vkGetRayTracingShaderGroupHandlesKHR"));
        vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(
                device, "vkCreateRayTracingPipelinesKHR"));
#if USE_MESH_SHADER
#if NV_MESH_SHADER
        vkCmdDrawMeshTasksNV = reinterpret_cast<PFN_vkCmdDrawMeshTasksNV>(vkGetDeviceProcAddr(
                device, "vkCmdDrawMeshTasksNV"));
#elif EXT_MESH_SHADER
        vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(
                device, "vkCmdDrawMeshTasksEXT"));
#endif
#endif
#if USE_VRS
        if (!vkCreateRenderPass2KHR) {
            vkCreateRenderPass2KHR = reinterpret_cast<PFN_vkCreateRenderPass2KHR>(vkGetInstanceProcAddr(instance, "vkCreateRenderPass2KHR"));
        }
        if (!vkCmdSetFragmentShadingRateKHR) {
            vkCmdSetFragmentShadingRateKHR = reinterpret_cast<PFN_vkCmdSetFragmentShadingRateKHR>(vkGetDeviceProcAddr(device, "vkCmdSetFragmentShadingRateKHR"));
        }
        if (!vkGetPhysicalDeviceFragmentShadingRatesKHR) {
            vkGetPhysicalDeviceFragmentShadingRatesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFragmentShadingRatesKHR"));
        }
#endif
    }

    void VulkanDevice::CreateInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "MWEngine Renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "MWEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance));
    }

    void VulkanDevice::pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        std::cout << "Device count: " << deviceCount << std::endl;
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto &device: devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        vkGetPhysicalDeviceFeatures(physicalDevice, &enabledFeatures);
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        std::cout << "physical renderDevice: " << properties.deviceName << std::endl;
    }

    void VulkanDevice::CreateLogicalDevice() {
        // Ray tracing related extensions required by this sample
        deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

        // Required by VK_KHR_acceleration_structure
        deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

        // Required for VK_KHR_ray_tracing_pipeline
        deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

        // Required by VK_KHR_spirv_1_4
        deviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

#if USE_MESH_SHADER
        deviceExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
#endif
#if USE_VRS
        deviceExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
#endif

        queueIndices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {queueIndices.graphicsFamily.value(),
                                                  queueIndices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily: uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // might not really be necessary anymore because renderDevice specific validation layers
        // have been deprecated
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }
        // If a pNext(Chain) has been passed, we need to add it to the device creation info
        if (deviceCreatepNextChain) {
            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};

            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.features = enabledFeatures;
            physicalDeviceFeatures2.pNext = deviceCreatepNextChain;
            createInfo.pEnabledFeatures = nullptr;
            createInfo.pNext = &physicalDeviceFeatures2;
        }
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical renderDevice!");
        }

        vkGetDeviceQueue(device, queueIndices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueIndices.presentFamily.value(), 0, &presentQueue);
    }

    void VulkanDevice::CreateCommandPool() {
        {
            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = queueIndices.graphicsFamily.value();
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            VK_CHECK_RESULT(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
        }

        {
            VkCommandPoolCreateInfo command_pool_create_info;
            command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            command_pool_create_info.pNext = NULL;
            command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            command_pool_create_info.queueFamilyIndex = queueIndices.graphicsFamily.value();

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                VK_CHECK_RESULT(
                        vkCreateCommandPool(device, &command_pool_create_info, NULL, &swapChainCommandPools[i]));
            }
        }
    }

    void VulkanDevice::CreateCommandBuffers() {
        VkCommandBufferAllocateInfo command_buffer_allocate_info{};
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1U;

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            command_buffer_allocate_info.commandPool = swapChainCommandPools[i];
            VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &commandBuffers[i]));
        }
    }

    void VulkanDevice::CreateSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("glfwCreateWindowSurface failed!");
        }
    }

    bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        return indices.isComplete() && extensionsSupported && swapChainAdequate &&
               supportedFeatures.samplerAnisotropy;
    }

    void VulkanDevice::populateDebugMessengerCreateInfo(
            VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;  // Optional
    }

    void VulkanDevice::setupDebugMessenger() {
        if (!enableValidationLayers) return;
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    bool VulkanDevice::checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName: validationLayers) {
            bool layerFound = false;

            for (const auto &layerProperties: availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    std::vector<const char *> VulkanDevice::getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#if USE_MESH_SHADER
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(
                device,
                nullptr,
                &extensionCount,
                availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &extension: availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily: queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) // if support graphics command queue
            {
                indices.graphicsFamily = i;
            }

            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) // if support compute command queue
            {
                indices.computeFamily = i;
            }


            VkBool32 is_present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device,
                                                 i,
                                                 surface,
                                                 &is_present_support); // if support surface presentation
            if (is_present_support) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }
            i++;
        }
        return indices;
    }

    SwapChainSupportDetails VulkanDevice::querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                    device,
                    surface,
                    &presentModeCount,
                    details.presentModes.data());
        }
        return details;
    }

    VkFormat VulkanDevice::findSupportedFormat(
            const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features,
            bool checkSamplingSupport) {
        for (VkFormat format: candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (checkSamplingSupport) {
                if (!(props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
                    continue;
                }
            }
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (
                    tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported format!");
    }

    uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void VulkanDevice::CreateBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer &buffer,
            VkDeviceMemory &bufferMemory,
            const void *pNext) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.pNext = pNext;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    uint64_t VulkanDevice::getBufferDeviceAddress(VkBuffer &buffer) {
        VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
        bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bufferDeviceAI.buffer = buffer;
        return vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
    }

    void VulkanDevice::getEnabledFeatures() {
        // Enable features required for ray tracing using feature chaining via pNext
        enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;
        enabledBufferDeviceAddresFeatures.pNext = &enabledRayTracingPipelineFeatures;

        enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
        enabledRayTracingPipelineFeatures.pNext = &enabledAccelerationStructureFeatures;

        enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
#if USE_VRS | 1
        enabledAccelerationStructureFeatures.pNext = &enabledPhysicalDeviceShadingRateImageFeaturesKHR;
        enabledPhysicalDeviceShadingRateImageFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
        enabledPhysicalDeviceShadingRateImageFeaturesKHR.attachmentFragmentShadingRate = VK_TRUE;
        enabledPhysicalDeviceShadingRateImageFeaturesKHR.pipelineFragmentShadingRate = VK_TRUE;
        enabledPhysicalDeviceShadingRateImageFeaturesKHR.primitiveFragmentShadingRate = VK_TRUE;
#endif
#if USE_MESH_SHADER | 1
        enabledPhysicalDeviceShadingRateImageFeaturesKHR.pNext = &enabledMeshShaderFeatures;
        enabledMeshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
        enabledMeshShaderFeatures.meshShader = VK_TRUE;
        enabledMeshShaderFeatures.taskShader = VK_TRUE;
#endif
        deviceCreatepNextChain = &enabledBufferDeviceAddresFeatures;
    }

    void VulkanDevice::CreateDeviceBuffer(VkBufferUsageFlags usageFlags,
                                          VulkanBuffer &buffer, VkDeviceSize size, void *data) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                     stagingBufferMemory);

        if (data != nullptr) {
            void *tempData;
            vkMapMemory(device, stagingBufferMemory, 0, size, 0, &tempData);
            memcpy(tempData, data, (size_t) size);
            vkUnmapMemory(device, stagingBufferMemory);
        }

        CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.buffer, buffer.memory);

        copyBuffer(stagingBuffer, buffer.buffer, size);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void VulkanDevice::CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
                                    VulkanBuffer &buffer,
                                    VkDeviceSize size, void *data) {
        // Create the buffer handle
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usageFlags;

        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer.buffer));

        // Create the memory backing up the buffer handle
        VkMemoryRequirements memReqs;
        VkMemoryAllocateInfo memAllocInfo{};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkGetBufferMemoryRequirements(device, buffer.buffer, &memReqs);
        memAllocInfo.allocationSize = memReqs.size;
        // Find a memory type index that fits the properties of the buffer
        memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
        // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
        if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
            allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
            allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            memAllocInfo.pNext = &allocFlagsInfo;
        }
        VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &buffer.memory));

        // If a pointer to the buffer data has been passed, map the buffer and copy over the data
        if (data != nullptr) {
            void *mapped;
            VK_CHECK_RESULT(vkMapMemory(device, buffer.memory, 0, size, 0, &mapped));
            memcpy(mapped, data, size);
            // If host coherency hasn't been requested, do a manual flush to make writes visible
            if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
                VkMappedMemoryRange mappedMemoryRange{};
                mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                mappedMemoryRange.memory = buffer.memory;
                mappedMemoryRange.offset = 0;
                mappedMemoryRange.size = size;
                vkFlushMappedMemoryRanges(device, 1, &mappedMemoryRange);
            }
            vkUnmapMemory(device, buffer.memory);
        }
        VK_CHECK_RESULT(vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0));
        buffer.bufferSize = size;
        buffer.usageFlags = usageFlags;
        buffer.memoryPropertyFlags = memoryPropertyFlags;

        buffer.setupDescriptor();
    }

    void VulkanDevice::flushBuffer(VulkanBuffer &buffer, VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = buffer.memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        VK_CHECK_RESULT(vkFlushMappedMemoryRanges(device, 1, &mappedRange));
    }

    void VulkanDevice::unMapMemory(VulkanBuffer &buffer) {
        if (buffer.mapped) {
            vkUnmapMemory(device, buffer.memory);
            buffer.mapped = nullptr;
        }
    }

    VkCommandBuffer VulkanDevice::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        endSingleTimeCommands(commandBuffer, graphicsQueue);
    }

    void VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void VulkanDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }


    void VulkanDevice::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
                                         uint32_t layerCount, VkBufferImageCopy *region) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        vkCmdCopyBufferToImage(
                commandBuffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                region);
        endSingleTimeCommands(commandBuffer);
    }

    void VulkanDevice::copyBufferToImage(
            VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount) {

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = layerCount;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};
        copyBufferToImage(buffer, image, width, height, layerCount, &region);
    }

    void VulkanDevice::CreateImageWithInfo(
            const VkImageCreateInfo &imageInfo,
            VkMemoryPropertyFlags propertiesFlags,
            VkImage &image,
            VkDeviceMemory &imageMemory) {
        VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, propertiesFlags);

        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));

        VK_CHECK_RESULT(vkBindImageMemory(device, image, imageMemory, 0));
    }

    void VulkanDevice::recreateSwapChain() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) // minimized 0,0, pause for now
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkWaitForFences(device, MAX_FRAMES_IN_FLIGHT, inFlightFences, VK_TRUE, UINT64_MAX);

        DestroyImageView(depthImageViews);
        DestroyImage(depthImages);
        FreeMemory(depthImageMemorys);
#if USE_VRS
        DestroyImageView(shadingRateImageViews);
        DestroyImage(shadingRateImages);
        FreeMemory(shadingRateImageMemorys);
#endif
        for (int i = 0; i < swapChainImageViews.size(); ++i) {
            DestroyImageView(swapChainImageViews[i]);
//            DestroyImage(swapChainImages[i]);
        }
        vkDestroySwapchainKHR(device, swapChain, NULL);

        createSwapChain();
        createSwapChainImageViews();
#if USE_VRS
        createShadingRateImage();
#endif
        createDepthResources();
    }

    void VulkanDevice::CreateSemaphore(
            const VkSemaphoreCreateInfo *pCreateInfo, VkSemaphore *pSemaphore,
            const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore));
    }

    void VulkanDevice::CreateFence(const VkFenceCreateInfo *pCreateInfo,
                                   VkFence *pFence,
                                   const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateFence(device, pCreateInfo, pAllocator, pFence));
    }

    void
    VulkanDevice::CreateImageView(const VkImageViewCreateInfo *pCreateInfo,
                                  VkImageView *pView, const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateImageView(device, pCreateInfo, pAllocator, pView));
    }

    void VulkanDevice::CreateSwapchainKHR(const VkSwapchainCreateInfoKHR *pCreateInfo, VkSwapchainKHR *pSwapchain,
                                          const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain));
    }

    void VulkanDevice::GetSwapchainImagesKHR(VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount,
                                             VkImage *pSwapchainImages) {
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages));
    }

    void VulkanDevice::QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) {
        VK_CHECK_RESULT(vkQueueSubmit(queue, submitCount, pSubmits, fence));
    }

    void VulkanDevice::ResetFences(uint32_t fenceCount, const VkFence *pFences) {
        vkResetFences(device, fenceCount, pFences);
    }

    VkResult
    VulkanDevice::AcquireNextImageKHR(VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence,
                                      uint32_t *pImageIndex) {
        return vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
    }

    void VulkanDevice::WaitForFences(uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout) {
        VK_CHECK_RESULT(vkWaitForFences(device, fenceCount, pFences, waitAll, timeout));
    }

    void VulkanDevice::DestroyImageView(VkImageView imageView, const VkAllocationCallbacks *pAllocator) {
        vkDestroyImageView(device, imageView, pAllocator);
    }

    void VulkanDevice::DestroyImage(VkImage image, const VkAllocationCallbacks *pAllocator) {
        vkDestroyImage(device, image, pAllocator);
    }

    void VulkanDevice::DestroySampler(VkSampler sampler, const VkAllocationCallbacks *pAllocator) {
        vkDestroySampler(device, sampler, pAllocator);
    }

    void VulkanDevice::FreeMemory(VkDeviceMemory deviceMemory, const VkAllocationCallbacks *pAllocator) {
        vkFreeMemory(device, deviceMemory, pAllocator);
    }

    void VulkanDevice::DestroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator) {
        vkDestroyFramebuffer(device, framebuffer, pAllocator);
    }

    void VulkanDevice::DestroySemaphore(VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator) {
        vkDestroySemaphore(device, semaphore, pAllocator);
    }

    void VulkanDevice::DestroyRenderPass(VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator) {
        vkDestroyRenderPass(device, renderPass, pAllocator);
    }

    void VulkanDevice::DestroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) {
        vkDestroyPipeline(device, pipeline, pAllocator);
    }

    void VulkanDevice::DestroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator) {
        vkDestroyPipelineLayout(device, pipelineLayout, pAllocator);
    }

    void VulkanDevice::DestroyFence(VkFence fence, const VkAllocationCallbacks *pAllocator) {
        vkDestroyFence(device, fence, pAllocator);
    }

    void VulkanDevice::DestroyShaderModule(VkShaderModule module, const VkAllocationCallbacks *pAllocator) {
        vkDestroyShaderModule(device, module, pAllocator);
    }

    void VulkanDevice::DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout,
                                                  const VkAllocationCallbacks *pAllocator) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
    }

    void VulkanDevice::DestroySwapchainKHR(VkSwapchainKHR swapChain, const VkAllocationCallbacks *pAllocator) {
        vkDestroySwapchainKHR(device, swapChain, pAllocator);
    }

    void VulkanDevice::CreateCommandPool(const VkCommandPoolCreateInfo *pCreateInfo, VkCommandPool *pCommandPool,
                                         const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool));
    }

    void VulkanDevice::CreateSampler(const VkSamplerCreateInfo *pCreateInfo, VkSampler *pSampler,
                                     const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateSampler(device, pCreateInfo, pAllocator, pSampler));
    }

    void VulkanDevice::CreateRenderPass(const VkRenderPassCreateInfo *pCreateInfo, VkRenderPass *pRenderPass,
                                        const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass));

    }

    void VulkanDevice::CreatePipelineLayout(const VkPipelineLayoutCreateInfo *pCreateInfo,
                                            VkPipelineLayout *pPipelineLayout,
                                            const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout));
    }

    void VulkanDevice::CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t CreateInfoCount,
                                               const VkGraphicsPipelineCreateInfo *pCreateInfos,
                                               VkPipeline *pPipelines, const VkAllocationCallbacks *pAllocator) {

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, CreateInfoCount, pCreateInfos, pAllocator,
                                                  pPipelines));
    }

    void
    VulkanDevice::CreateGraphicsPipelines(const VkGraphicsPipelineCreateInfo *pCreateInfos, VkPipeline *pPipelines) {
        CreateGraphicsPipelines(VK_NULL_HANDLE, 1, pCreateInfos, pPipelines, nullptr);
    }

    void VulkanDevice::CreateRayTracingPipelinesKHR(const VkRayTracingPipelineCreateInfoKHR *pCreateInfos,
                                                    VkPipeline *pPipelines,
                                                    uint32_t createInfoCount,
                                                    VkDeferredOperationKHR deferredOperation,
                                                    VkPipelineCache pipelineCache,
                                                    const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache,
                                                       createInfoCount, pCreateInfos, pAllocator, pPipelines));
    }

    void VulkanDevice::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                 VkDescriptorSetLayout *pSetLayout,
                                                 const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout));
    }

    void VulkanDevice::CreateDescriptorSet(uint32_t descriptorSetCount, const VkDescriptorSetLayout &pSetLayouts,
                                           VkDescriptorSet &set) {
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.pSetLayouts = &pSetLayouts;
        descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
        AllocateDescriptorSets(&descriptorSetAllocateInfo, &set);
    }

    void VulkanDevice::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                              VkDescriptorSet *pDescriptorSets) {
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets));
    }

    void VulkanDevice::UpdateDescriptorSets(uint32_t descriptorWriteCount,
                                            const VkWriteDescriptorSet *pDescriptorWrites,
                                            uint32_t descriptorCopyCount,
                                            const VkCopyDescriptorSet *pDescriptorCopies) {
        vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    }

    void VulkanDevice::CreateFramebuffer(const VkFramebufferCreateInfo *pCreateInfo, VkFramebuffer *pFramebuffer,
                                         const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer));
    }

    void
    VulkanDevice::CreateDescriptorPool(const VkDescriptorPoolCreateInfo *pCreateInfo, VkDescriptorPool *pDescriptorPool,
                                       const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool));
    }

//    void VulkanDevice::CreateImageView(VkImage *image, VkFormat format, VkImageAspectFlags image_aspect_flags,
//                                       VkImageViewType view_type, uint32_t layout_count, uint32_t miplevels,
//                                       VkImageView *image_view) {

//        VK_CHECK_RESULT(vkCreateImageView());
//    }


    VkResult VulkanDevice::acquireNextImage(uint32_t *imageIndex) {
        WaitForFences(
                1,
                &inFlightFences[currentFrameIndex],
                VK_TRUE,
                std::numeric_limits<uint64_t>::max());

        VkResult result = AcquireNextImageKHR(
                swapChain,
                std::numeric_limits<uint64_t>::max(),
                imageAvailableSemaphores[currentFrameIndex],  // must be a not signaled semaphore
                VK_NULL_HANDLE,
                imageIndex);

        return result;
    }

    void VulkanDevice::submitCommandBuffers(std::function<void()> passUpdateAfterRecreateSwapchain) {
        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers[currentFrameIndex]));
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrameIndex]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrameIndex];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrameIndex]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        ResetFences(1, &inFlightFences[currentFrameIndex]);
        QueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrameIndex]);

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &currentImageIndex;

        auto result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (VK_ERROR_OUT_OF_DATE_KHR == result || VK_SUBOPTIMAL_KHR == result) {
            recreateSwapChain();
            passUpdateAfterRecreateSwapchain();
        } else {
            if (VK_SUCCESS != result) {
                std::cerr << (("vkQueuePresentKHR failed!")) << std::endl;
                return;
            }
        }
        currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanDevice::waitForFences() {
        VK_CHECK_RESULT(vkWaitForFences(device, 1, &inFlightFences[currentFrameIndex], VK_TRUE, UINT64_MAX));
    }

    void VulkanDevice::resetCommandPool() {
        VK_CHECK_RESULT(vkResetCommandPool(device, swapChainCommandPools[currentFrameIndex], 0));
    }

    bool VulkanDevice::prepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapchain) {
        waitForFences();

        resetCommandPool();

        VkResult acquire_image_result =
                vkAcquireNextImageKHR(device,
                                      swapChain,
                                      UINT64_MAX,
                                      imageAvailableSemaphores[currentFrameIndex],
                                      VK_NULL_HANDLE,
                                      &currentImageIndex);

        if (VK_ERROR_OUT_OF_DATE_KHR == acquire_image_result) {
            recreateSwapChain();
            passUpdateAfterRecreateSwapchain();
            return VK_SUCCESS;
        } else if (VK_SUBOPTIMAL_KHR == acquire_image_result) {
            recreateSwapChain();
            passUpdateAfterRecreateSwapchain();

            // NULL submit to wait semaphore
            VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
            VkSubmitInfo submit_info = {};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &imageAvailableSemaphores[currentFrameIndex];
            submit_info.pWaitDstStageMask = wait_stages;
            submit_info.commandBufferCount = 0;
            submit_info.pCommandBuffers = NULL;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = NULL;

            VK_CHECK_RESULT(vkResetFences(device, 1, &inFlightFences[currentFrameIndex]));

            VkResult res_queue_submit =
                    vkQueueSubmit(graphicsQueue, 1, &submit_info, inFlightFences[currentFrameIndex]);
            if (VK_SUCCESS != res_queue_submit) {
                std::cerr << ("vkQueueSubmit failed!") << std::endl;;
                return false;
            }
            currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
            return VK_SUCCESS;
        } else {
            if (VK_SUCCESS != acquire_image_result) {
                std::cerr << ("vkAcquireNextImageKHR failed!") << std::endl;
                return false;
            }
        }

        // begin command buffer
        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        VkResult res_begin_command_buffer =
                vkBeginCommandBuffer(commandBuffers[currentFrameIndex], &command_buffer_begin_info);

        if (VK_SUCCESS != res_begin_command_buffer) {
            std::cerr << ("_vkBeginCommandBuffer failed!") << std::endl;
            return false;
        }
        return false;
    }

    void VulkanDevice::createSwapChain() {
        SwapChainSupportDetails swapChainSupport = getSwapChainSupport();

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 &&
            imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (swapChainSupport.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        // Enable transfer destination on swap chain images if supported
        if (swapChainSupport.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        QueueFamilyIndices indices = findPhysicalQueueFamilies();
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;      // Optional
            createInfo.pQueueFamilyIndices = nullptr;  // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        CreateSwapchainKHR(&createInfo, &swapChain);

        // we only specified a minimum number of images in the swap chain, so the implementation is
        // allowed to create a swap chain with more. That's why we'll first query the final number of
        // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
        // retrieve the handles.
        GetSwapchainImagesKHR(swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        GetSwapchainImagesKHR(swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
        swapChainScissor = {{0,                     0},
                            {swapChainExtent.width, swapChainExtent.height}};
    }

    void VulkanDevice::createSwapChainImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapChainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapChainImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            CreateImageView(&viewInfo, &swapChainImageViews[i]);
        }
    }

    VulkanSwapChainDesc VulkanDevice::getSwapchainInfo() {
        VulkanSwapChainDesc desc;
        desc.imageFormat = swapChainImageFormat;
        desc.extent = swapChainExtent;
        desc.viewport = &swapChainViewport;
        desc.scissor = &swapChainScissor;
        desc.imageViews = swapChainImageViews;
        desc.images = swapChainImages;
        return desc;
    }

    VulkanDepthImageDesc VulkanDevice::getDepthImageInfo() const {
        VulkanDepthImageDesc desc;
        desc.depthImageFormat = swapChainDepthFormat;
        desc.depthImageView = depthImageViews;
        desc.depthImage = depthImages;
        return desc;
    }

#if USE_VRS
    void VulkanDevice::CreateRenderPass2KHR(const VkRenderPassCreateInfo2KHR *pCreateInfo, VkRenderPass *pRenderPass,
                                        const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateRenderPass2KHR(device, pCreateInfo, pAllocator, pRenderPass));

    }

    VulkanShadingRateImageDesc VulkanDevice::getShadingRateImageInfo() {
        VulkanShadingRateImageDesc desc;
        desc.shadingRateImage = shadingRateImages;
        desc.shadingRateImageView = shadingRateImageViews;
        desc.extent = swapChainShadingRateExtent;
        desc.shadingRateImageFormat = swapChainShadingRateFormat;
        return desc;
    }

    void VulkanDevice::updateShadingRateImage() {
        const auto &imageExtent = swapChainShadingRateExtent;
// The shading rates are stored in a buffer that'll be copied to the shading rate image
        VkDeviceSize bufferSize = imageExtent.width * imageExtent.height * sizeof(uint8_t);

        // Fragment sizes are encoded in a single texel as follows:
        // size(w) = 2^((texel/4) & 3)
        // size(h) = 2^(texel & 3)

        // Populate it with the lowest possible shading rate
        uint8_t  val = (4 >> 1) | (4 << 1);
        uint8_t* shadingRatePatternData = new uint8_t[bufferSize];
        memset(shadingRatePatternData, val, bufferSize);

        // Get a list of available shading rate patterns
        std::vector<VkPhysicalDeviceFragmentShadingRateKHR> fragmentShadingRates{};
        uint32_t fragmentShadingRatesCount = 0;
        vkGetPhysicalDeviceFragmentShadingRatesKHR(physicalDevice, &fragmentShadingRatesCount, nullptr);
        if (fragmentShadingRatesCount > 0) {
            fragmentShadingRates.resize(fragmentShadingRatesCount);
            for (VkPhysicalDeviceFragmentShadingRateKHR& fragmentShadingRate : fragmentShadingRates) {
                // In addition to the value, we also need to set the sType for each rate to comply with the spec or else the call to vkGetPhysicalDeviceFragmentShadingRatesKHR will result in undefined behaviour
                fragmentShadingRate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR;
            }
            vkGetPhysicalDeviceFragmentShadingRatesKHR(physicalDevice, &fragmentShadingRatesCount, fragmentShadingRates.data());
        }
        // Create a circular pattern from the available list of fragment shading rates with decreasing sampling rates outwards (max. range, pattern)
        // Shading rates returned by vkGetPhysicalDeviceFragmentShadingRatesKHR are ordered from largest to smallest
        std::map<float, uint8_t> patternLookup{};
        float range = 25.0f / static_cast<uint32_t>(fragmentShadingRates.size());
        float currentRange = 8.0f;
        for (size_t i = fragmentShadingRates.size() - 1; i > 0; i--) {
            uint32_t rate_v = fragmentShadingRates[i].fragmentSize.width >> 1;
            uint32_t rate_h = fragmentShadingRates[i].fragmentSize.height << 1;
            patternLookup[currentRange] = rate_v | rate_h;
            currentRange += range;
        }

        uint8_t* ptrData = shadingRatePatternData;
        for (uint32_t y = 0; y < imageExtent.height; y++) {
            for (uint32_t x = 0; x < imageExtent.width; x++) {
                const float deltaX = (static_cast<float>(imageExtent.width) / 2.0f - static_cast<float>(x)) / imageExtent.width * 100.0f;
                const float deltaY = (static_cast<float>(imageExtent.height) / 2.0f - static_cast<float>(y)) / imageExtent.height * 100.0f;
                const float dist = std::sqrt(deltaX * deltaX + deltaY * deltaY);
                for (auto pattern : patternLookup) {
                    if (dist < pattern.first) {
                        *ptrData = pattern.second;
                        break;
                    }
                }
                ptrData++;
            }
        }

        // Copy the shading rate pattern to the shading rate image

        VulkanBuffer stagingBuffer;
        CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, bufferSize);

        MapMemory(stagingBuffer);
        memcpy(stagingBuffer.mapped, shadingRatePatternData, bufferSize);
        unMapMemory(stagingBuffer);
        delete[] shadingRatePatternData;

        // Upload
        VkCommandBuffer copyCmd = beginSingleTimeCommands();
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;
        transitionImageLayout(
                copyCmd,
                shadingRateImages,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                subresourceRange);
        VkBufferImageCopy bufferCopyRegion{};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = imageExtent.width;
        bufferCopyRegion.imageExtent.height = imageExtent.height;
        bufferCopyRegion.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(copyCmd, stagingBuffer.buffer, shadingRateImages, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
        transitionImageLayout(
                copyCmd,
                shadingRateImages,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR,
                subresourceRange);

        endSingleTimeCommands(copyCmd);
        DestroyVulkanBuffer(stagingBuffer);
    }
#endif

    void VulkanDevice::createDepthResources() {
        VkFormat depthFormat = findDepthFormat();
        swapChainDepthFormat = depthFormat;
        VkExtent2D swapChainExtent = getSwapChainExtent();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;

        CreateImageWithInfo(
                imageInfo,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthImages,
                depthImageMemorys);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImages;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        CreateImageView(&viewInfo, &depthImageViews);
    }
#if USE_VRS
    void VulkanDevice::createShadingRateImage() {
        swapChainShadingRateFormat = VK_FORMAT_R8_UINT;
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, swapChainShadingRateFormat, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR))
        {
            throw std::runtime_error("Selected shading rate attachment image format does not fragment shading rate");
        }
        // Shading rate image size depends on shading rate texel size
        // For each texel in the target image, there is a corresponding shading texel size width x height block in the shading rate image
        const auto &swapchainInfo = getSwapchainInfo();

        swapChainShadingRateExtent.width = static_cast<uint32_t>(ceil(swapchainInfo.extent.width /
                                                       (float) physicalDeviceShadingRateImageProperties.maxFragmentShadingRateAttachmentTexelSize.width));
        swapChainShadingRateExtent.height = static_cast<uint32_t>(ceil(swapchainInfo.extent.height /
                                                        (float) physicalDeviceShadingRateImageProperties.maxFragmentShadingRateAttachmentTexelSize.height));

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = swapChainShadingRateFormat;
        imageInfo.extent.width = swapChainShadingRateExtent.width;
        imageInfo.extent.height = swapChainShadingRateExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.usage = VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        CreateImageWithInfo(
                imageInfo,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                shadingRateImages,
                shadingRateImageMemorys);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = shadingRateImages;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapChainShadingRateFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        CreateImageView(&viewInfo, &shadingRateImageViews);
        updateShadingRateImage();
    }
#endif
    void VulkanDevice::createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            CreateSemaphore(&semaphoreInfo, &imageAvailableSemaphores[i]);
            CreateSemaphore(&semaphoreInfo, &renderFinishedSemaphores[i]);
            CreateFence(&fenceInfo, &inFlightFences[i]);
        }
    }

    VkSurfaceFormatKHR VulkanDevice::chooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        for (const auto &availableFormat: availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VulkanDevice::chooseSwapPresentMode(
            const std::vector<VkPresentModeKHR> &availablePresentModes) {
        for (const auto &availablePresentMode: availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                std::cout << "Present mode: Mailbox" << std::endl;
                return availablePresentMode;
            }
        }

        // for (const auto &availablePresentMode : availablePresentModes) {
        //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        //     std::cout << "Present mode: Immediate" << std::endl;
        //     return availablePresentMode;
        //   }
        // }

        std::cout << "Present mode: V-Sync" << std::endl;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanDevice::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D actualExtent = windowExtent;
            actualExtent.width = std::max(
                    capabilities.minImageExtent.width,
                    std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(
                    capabilities.minImageExtent.height,
                    std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    VkFormat VulkanDevice::findDepthFormat(bool checkSamplingSupport) {
        return findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, checkSamplingSupport);
    }

    void VulkanDevice::updateDescriptorSets(const VkWriteDescriptorSet *pDescriptorWrites,
                                            uint32_t descriptorWriteCount,
                                            const VkCopyDescriptorSet *pDescriptorCopies,
                                            uint32_t descriptorCopyCount) {
        vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount,
                               pDescriptorCopies);
    }

    void VulkanDevice::CreateDescriptorPool() {
        std::array<VkDescriptorPoolSize, 8> pool_sizes{};
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        pool_sizes[0].descriptorCount = 64;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_sizes[1].descriptorCount = 64;
        pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[2].descriptorCount = 1 * maxMaterialCount;
        pool_sizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[3].descriptorCount = 1 * maxMaterialCount;
        pool_sizes[4].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        pool_sizes[4].descriptorCount = 64;
        pool_sizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        pool_sizes[5].descriptorCount = 64;
        pool_sizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        pool_sizes[6].descriptorCount = 64;
        pool_sizes[7].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        pool_sizes[7].descriptorCount = 64;

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = pool_sizes.size();
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = 1 + 1 + 1 + maxMaterialCount + 1 + 1 + 1; // +skybox + axis descriptor set
        pool_info.flags = 0U;

        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool));
    }

    VkShaderModule VulkanDevice::CreateShaderModule(const std::vector<unsigned char> &shader_code) {
        VkShaderModuleCreateInfo shader_module_create_info{};
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.codeSize = shader_code.size();
        shader_module_create_info.pCode = reinterpret_cast<const uint32_t *>(shader_code.data());

        VkShaderModule shader_module;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &shader_module_create_info, nullptr, &shader_module));
        return shader_module;
    }

    void VulkanDevice::GetPhysicalDeviceProperties2(VkPhysicalDeviceProperties2 *pProperties) {
        vkGetPhysicalDeviceProperties2(physicalDevice, pProperties);
    }

    void VulkanDevice::GetPhysicalDeviceFeatures2(VkPhysicalDeviceFeatures2 *pFeatures) {
        vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
    }

    void VulkanDevice::GetPhysicalDeviceFormatProperties(VkFormat format, VkFormatProperties *pFormatProperties) {
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
    }

    void VulkanDevice::createAccelerationStructureBuffer(AccelerationStructure &accelerationStructure,
                                                         VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo) {
        VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
        memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        CreateBuffer(buildSizeInfo.accelerationStructureSize,
                     VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, accelerationStructure.buffer, accelerationStructure.memory,
                     &memoryAllocateFlagsInfo);
    }

    void VulkanDevice::GetAccelerationStructureBuildSizesKHR(VkAccelerationStructureBuildTypeKHR buildType,
                                                             const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo,
                                                             const uint32_t *pMaxPrimitiveCounts,
                                                             VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo) {
        vkGetAccelerationStructureBuildSizesKHR(
                device,
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                pBuildInfo,
                pMaxPrimitiveCounts,
                pSizeInfo);
    }

    void VulkanDevice::CreateAccelerationStructureKHR(const VkAccelerationStructureCreateInfoKHR *pCreateInfo,
                                                      VkAccelerationStructureKHR *pAccelerationStructure,
                                                      const VkAllocationCallbacks *pAllocator) {
        VK_CHECK_RESULT(vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure));
    }

    RayTracingScratchBuffer VulkanDevice::createScratchBuffer(VkDeviceSize size) {
        RayTracingScratchBuffer scratchBuffer{};

        VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
        memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        CreateBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     scratchBuffer.handle, scratchBuffer.memory, &memoryAllocateFlagsInfo);
        scratchBuffer.deviceAddress = getBufferDeviceAddress(scratchBuffer.handle);

        return scratchBuffer;
    }

    void VulkanDevice::DestroyScratchBuffer(RayTracingScratchBuffer &scratchBuffer) {
        if (scratchBuffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, scratchBuffer.memory, nullptr);
        }
        if (scratchBuffer.handle != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, scratchBuffer.handle, nullptr);
        }
    }

    void VulkanDevice::DestroyVkBuffer(VkBuffer &vulkanBuffer) {
        if (vulkanBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, vulkanBuffer, nullptr);
        }
    }

    void VulkanDevice::DestroyVulkanBuffer(VulkanBuffer &vulkanBuffer) {
        if (vulkanBuffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, vulkanBuffer.memory, nullptr);
        }
        if (vulkanBuffer.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, vulkanBuffer.buffer, nullptr);
        }
    }

    uint32_t
    VulkanDevice::getAccelerationStructureDeviceAddressKHR(const VkAccelerationStructureDeviceAddressInfoKHR *pInfo) {
        return vkGetAccelerationStructureDeviceAddressKHR(device, pInfo);
    }

    void VulkanDevice::GetRayTracingShaderGroupHandlesKHR(VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount,
                                                          size_t dataSize, void *pData) {
        VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, dataSize, pData));
    }

    void VulkanDevice::CmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount,
                                                         const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
                                                         const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos) {
        vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
    }

    void VulkanDevice::CmdTraceRaysKHR(VkCommandBuffer commandBuffer,
                                       const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable,
                                       const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable,
                                       const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable,
                                       const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable,
                                       uint32_t width, uint32_t height, uint32_t depth) {
        vkCmdTraceRaysKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable,
                          pCallableShaderBindingTable, width, height, depth);
    }

    void VulkanDevice::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                             bool useCurrentCommandBuffer,
                                             VkImageSubresourceRange subresourceRange,
                                             VkPipelineStageFlags srcStageMask,
                                             VkPipelineStageFlags dstStageMask) {
        VkCommandBuffer commandBuffer;
        if (useCurrentCommandBuffer)commandBuffer = getCurrentCommandBuffer();
        else commandBuffer = beginSingleTimeCommands();
        transitionImageLayout(commandBuffer, image, oldLayout, newLayout, subresourceRange, srcStageMask, dstStageMask);
        if (!useCurrentCommandBuffer)
            endSingleTimeCommands(commandBuffer);
    }

    void VulkanDevice::transitionImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldLayout,
                                             VkImageLayout newLayout, VkImageSubresourceRange subresourceRange,
                                             VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = subresourceRange;

        switch (oldLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                barrier.srcAccessMask = 0;
                srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image is a transfer source
                // Make sure any reads from the image have been finished
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newLayout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (barrier.srcAccessMask == 0) {
                    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        vkCmdPipelineBarrier(
                cmdbuffer,
                srcStageMask, dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
        );
    }

    void VulkanDevice::MapMemory(VulkanBuffer &vulkanBuffer, VkDeviceSize size, VkDeviceSize offset) {
        VK_CHECK_RESULT(vkMapMemory(device, vulkanBuffer.memory, offset, size, 0, &vulkanBuffer.mapped));
    }

    void VulkanDevice::MapMemory(VkDeviceMemory memory, void *mapped, VkDeviceSize size, VkDeviceSize offset) {
        VK_CHECK_RESULT(vkMapMemory(device, memory, offset, size, 0, &mapped));
    }

    VkSampler VulkanDevice::getOrCreateDefaultSampler(VkFilter filter) {
        switch (filter) {
            case VK_FILTER_NEAREST:
                if (defaultNearestSampler == VK_NULL_HANDLE)
                    createDefaultSampler(filter, &defaultNearestSampler);
                return defaultNearestSampler;
            case VK_FILTER_LINEAR:
                if (defaultLinearSampler == VK_NULL_HANDLE)
                    createDefaultSampler(filter, &defaultLinearSampler);
                return defaultLinearSampler;
            default:
                return nullptr;
        }
    }

    void VulkanDevice::createDefaultSampler(VkFilter filter, VkSampler *sampler) {
        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = filter;
        samplerInfo.minFilter = filter;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // close :1.0f
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 8.0f; // todo: m_irradiance_texture_miplevels
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        CreateSampler(&samplerInfo, sampler);
    }

    /**
    * Finish command buffer recording and submit it to a queue
    *
    * @param commandBuffer Command buffer to flush
    * @param queue Queue to submit the command buffer to
    * @param pool Command pool on which the command buffer has been created
    * @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
    *
    * @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
    * @note Uses a fence to ensure command buffer has finished executing
    */
    void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free) {
        if (commandBuffer == VK_NULL_HANDLE) {
            return;
        }

        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo = CreateFenceCreateInfo();
        VkFence fence;
        CreateFence(&fenceInfo, &fence);
        // Submit to the queue
        VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
        // Wait for the fence to signal that command buffer has finished executing
        WaitForFences(1, &fence);
        DestroyFence(fence);
        if (free) {
            vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
        }
    }

    void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free) {
        return flushCommandBuffer(commandBuffer, queue, commandPool, free);
    }

    void VulkanDevice::DestroyAccelerationStructureKHR(VkAccelerationStructureKHR accelerationStructure,
                                                       const VkAllocationCallbacks *pAllocator) {
        if (vkDestroyAccelerationStructureKHR) {
            vkDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
        }
    }
}