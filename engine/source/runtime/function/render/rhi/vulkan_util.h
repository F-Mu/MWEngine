#pragma once

#include <iostream>
#include <fstream>
namespace MW {
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
    const std::string getAssetPath()
    {
#if defined(ASSET_DIR)
        return ASSET_DIR;
#else
        return "./../../../../assets/";
#endif
    }
    inline VkImageCreateInfo CreateImageCreateInfo()
    {
        VkImageCreateInfo imageCreateInfo {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        return imageCreateInfo;
    }

    bool fileExists(const std::string &filename)
    {
        std::ifstream f(filename.c_str());
        return !f.fail();
    }

    void exitFatal(const std::string& message, int32_t exitCode)
    {
        std::cerr << message << "\n";
        exit(exitCode);
    }

    void exitFatal(const std::string& message, VkResult resultCode)
    {
        exitFatal(message, (int32_t)resultCode);
    }
    class VulkanUtil {
    public:
        static uint32_t alignedSize(uint32_t value, uint32_t alignment);
    };
} // namespace MW
