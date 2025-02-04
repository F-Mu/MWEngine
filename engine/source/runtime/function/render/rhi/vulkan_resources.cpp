#include "vulkan_resources.h"
#include "vulkan_device.h"
#include <cassert>

namespace MW {

    void VulkanTexture::updateDescriptor() {
        descriptor.sampler = sampler;
        descriptor.imageView = view;
        descriptor.imageLayout = imageLayout;
    }

    void VulkanTexture::destroy(std::shared_ptr<VulkanDevice> device) {

        device->DestroyImageView(view);
        device->DestroyImage(image);
        if (sampler)
            device->DestroySampler(sampler);
        device->FreeMemory(deviceMemory);
    }

    ktxResult VulkanTexture::loadKTXFile(std::string filename, ktxTexture **target) {
        ktxResult result = KTX_SUCCESS;
#if defined(__ANDROID__)
        AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
        if (!asset) {
            vks::tools::exitFatal("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
        }
        size_t size = AAsset_getLength(asset);
        assert(size > 0);
        ktx_uint8_t *textureData = new ktx_uint8_t[size];
        AAsset_read(asset, textureData, size);
        AAsset_close(asset);
        result = ktxTexture_CreateFromMemory(textureData, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, target);
        delete[] textureData;
#else
        if (!fileExists(filename)) {
            exitFatal("Could not load texture from " + filename +
                      "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
        }
        result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, target);
#endif
        return result;
    }

    void VulkanTexture2D::fromBuffer(void *buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth,
                                     uint32_t texHeight, std::shared_ptr<VulkanDevice> device,
                                     VkFilter filter, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout) {
        assert(buffer);

        width = texWidth;
        height = texHeight;
        mipLevels = 1;
        // Use a separate command buffer for texture loading
        VkCommandBuffer copyCmd = device->beginSingleTimeCommands();

        // Create a host-visible staging buffer that contains the raw image data
        VulkanBuffer stagingBuffer;
        device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, bufferSize, buffer);

//        device->MapMemory(stagingBuffer);
//        memcpy(stagingBuffer.mapped, buffer, bufferSize);
//        device->unMapMemory(stagingBuffer);

        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = 0;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = width;
        bufferCopyRegion.imageExtent.height = height;
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = 0;

        // Create optimal tiled target image
        VkImageCreateInfo imageCreateInfo = CreateImageCreateInfo();
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent = {width, height, 1};
        imageCreateInfo.usage = imageUsageFlags;
        // Ensure that the TRANSFER_DST bit is set for staging
        if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        device->CreateImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, deviceMemory);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = mipLevels;
        subresourceRange.layerCount = 1;

        device->transitionImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      subresourceRange);
        // Copy mip levels from staging buffer
        vkCmdCopyBufferToImage(copyCmd, stagingBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &bufferCopyRegion);

        // Change texture image layout to shader read after all mip levels have been copied
        this->imageLayout = imageLayout;
        device->transitionImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout,
                                      subresourceRange);

        device->endSingleTimeCommands(copyCmd);

        // Clean up staging resources
        device->DestroyVulkanBuffer(stagingBuffer);

        // Create sampler
        VkSamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = filter;
        samplerCreateInfo.minFilter = filter;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = 0.0f;
        samplerCreateInfo.maxAnisotropy = 1.0f;
        device->CreateSampler(&samplerCreateInfo, &sampler);

        // Create image view
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.pNext = NULL;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = format;
        viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.image = image;
        device->CreateImageView(&viewCreateInfo, &view);

        // Update descriptor image info member that can be used for setting up descriptor sets
        updateDescriptor();
    }

    void VulkanBuffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
        descriptor.offset = offset;
        descriptor.buffer = buffer;
        descriptor.range = size;
    }

    void VulkanTextureCubeMap::loadFromFile(std::string filename, VkFormat format, std::shared_ptr<VulkanDevice> device,
                                            VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout) {
        ktxTexture *ktxTexture;
        ktxResult result = loadKTXFile(filename, &ktxTexture);
        assert(result == KTX_SUCCESS);

        width = ktxTexture->baseWidth;
        height = ktxTexture->baseHeight;
        mipLevels = ktxTexture->numLevels;

        ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
        ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

        // Create a host-visible staging buffer that contains the raw image data
        VulkanBuffer stagingBuffer;
        device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, ktxTextureSize, ktxTextureData);

//        device->MapMemory(stagingBuffer);
//        memcpy(stagingBuffer.mapped, ktxTextureData, ktxTextureSize);
//        device->unMapMemory(stagingBuffer);

        // Setup buffer copy regions for each face including all of its mip levels
        std::vector<VkBufferImageCopy> bufferCopyRegions;

        for (uint32_t face = 0; face < 6; face++) {
            for (uint32_t level = 0; level < mipLevels; level++) {
                ktx_size_t offset;
                KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
                assert(result == KTX_SUCCESS);

                VkBufferImageCopy bufferCopyRegion = {};
                bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel = level;
                bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
                bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
                bufferCopyRegion.imageExtent.depth = 1;
                bufferCopyRegion.bufferOffset = offset;

                bufferCopyRegions.push_back(bufferCopyRegion);
            }
        }

        // Create optimal tiled target image
        VkImageCreateInfo imageCreateInfo = CreateImageCreateInfo();
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent = {width, height, 1};
        imageCreateInfo.usage = imageUsageFlags;
        // Ensure that the TRANSFER_DST bit is set for staging
        if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        // Cube faces count as array layers in Vulkan
        imageCreateInfo.arrayLayers = 6;
        // This flag is required for cube map images
        imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;


        device->CreateImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, deviceMemory);

        // Use a separate command buffer for texture loading
        VkCommandBuffer copyCmd = device->beginSingleTimeCommands();

        // Image barrier for optimal image (target)
        // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = mipLevels;
        subresourceRange.layerCount = 6;

        device->transitionImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      subresourceRange);
        // Copy mip levels from staging buffer
        vkCmdCopyBufferToImage(copyCmd, stagingBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               bufferCopyRegions.size(),
                               bufferCopyRegions.data());

        // Change texture image layout to shader read after all mip levels have been copied
        this->imageLayout = imageLayout;
        device->transitionImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout,
                                      subresourceRange);

        device->endSingleTimeCommands(copyCmd);
        // Create sampler
        VkSamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
        samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.maxAnisotropy = device->enabledFeatures.samplerAnisotropy
                                          ? device->properties.limits.maxSamplerAnisotropy : 1.0f;
        samplerCreateInfo.anisotropyEnable = device->enabledFeatures.samplerAnisotropy;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = (float) mipLevels;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        device->CreateSampler(&samplerCreateInfo, &sampler);

        // Create image view
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewCreateInfo.format = format;
        viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        viewCreateInfo.subresourceRange.layerCount = 6;
        viewCreateInfo.subresourceRange.levelCount = mipLevels;
        viewCreateInfo.image = image;
        device->CreateImageView(&viewCreateInfo, &view);

        // Clean up staging resources
        ktxTexture_Destroy(ktxTexture);
        device->DestroyVulkanBuffer(stagingBuffer);

        // Update descriptor image info member that can be used for setting up descriptor sets
        updateDescriptor();
    }

    void StorageImage::destroy(std::shared_ptr<VulkanDevice> device)  {
        device->DestroyImageView(view);
        device->DestroyImage(image);
        device->FreeMemory(memory);
    }

    void AccelerationStructure::destroy(std::shared_ptr<VulkanDevice> device)  {
        device->DestroyVkBuffer(buffer);
        device->DestroyAccelerationStructureKHR(handle);
        device->FreeMemory(memory);
    }
}