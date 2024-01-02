/*
* Vulkan glTF model and texture loading class based on tinyglTF (https://github.com/syoyo/tinygltf)
*
* Copyright (C) 2018-2023 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "render_model.h"

namespace MW {
    VkDescriptorSetLayout descriptorSetLayoutImage = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayoutUbo = VK_NULL_HANDLE;
    VkMemoryPropertyFlags memoryPropertyFlags = 0;
    uint32_t descriptorBindingFlags = DescriptorBindingFlags::ImageBaseColor;

/*
	We use a custom image loading function with tinyglTF, so we can do custom stuff loading ktx textures
*/
    bool loadImageDataFunc(tinygltf::Image *image, const int imageIndex, std::string *error, std::string *warning,
                           int req_width, int req_height, const unsigned char *bytes, int size, void *userData) {
        // KTX files will be handled by our own code
        if (image->uri.find_last_of(".") != std::string::npos) {
            if (image->uri.substr(image->uri.find_last_of(".") + 1) == "ktx") {
                return true;
            }
        }

        return tinygltf::LoadImageData(image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
    }

    bool loadImageDataFuncEmpty(tinygltf::Image *image, const int imageIndex, std::string *error, std::string *warning,
                                int req_width, int req_height, const unsigned char *bytes, int size, void *userData) {
        // This function will be used for samples that don't require images to be loaded
        return true;
    }


/*
	glTF texture loading class
*/

    void Texture::updateDescriptor() {
        descriptor.sampler = sampler;
        descriptor.imageView = view;
        descriptor.imageLayout = imageLayout;
    }

    void Texture::destroy() {
        if (device) {
            device->DestroyImageView(view);
            device->DestroyImage(image);
            device->FreeMemory(deviceMemory);
            device->DestroySampler(sampler);
        }
    }

    void Texture::fromglTfImage(tinygltf::Image &gltfimage, std::string path, VulkanDevice *device,
                                VkQueue copyQueue) {
        this->device = device;

        bool isKtx = false;
        // Image points to an external ktx file
        if (gltfimage.uri.find_last_of(".") != std::string::npos) {
            if (gltfimage.uri.substr(gltfimage.uri.find_last_of(".") + 1) == "ktx") {
                isKtx = true;
            }
        }

        VkFormat format;

        if (!isKtx) {
            // Texture was loaded using STB_Image

            unsigned char *buffer = nullptr;
            VkDeviceSize bufferSize = 0;
            bool deleteBuffer = false;
            if (gltfimage.component == 3) {
                // Most devices don't support RGB only on Vulkan so convert if necessary
                // TODO: Check actual format support and transform only if required
                bufferSize = gltfimage.width * gltfimage.height * 4;
                buffer = new unsigned char[bufferSize];
                unsigned char *rgba = buffer;
                unsigned char *rgb = &gltfimage.image[0];
                for (size_t i = 0; i < gltfimage.width * gltfimage.height; ++i) {
                    for (int32_t j = 0; j < 3; ++j) {
                        rgba[j] = rgb[j];
                    }
                    rgba += 4;
                    rgb += 3;
                }
                deleteBuffer = true;
            } else {
                buffer = &gltfimage.image[0];
                bufferSize = gltfimage.image.size();
            }

            format = VK_FORMAT_R8G8B8A8_UNORM;

            VkFormatProperties formatProperties;

            width = gltfimage.width;
            height = gltfimage.height;
            mipLevels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1.0);

            device->GetPhysicalDeviceFormatProperties(format, &formatProperties);
            assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
            assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

            VkMemoryAllocateInfo memAllocInfo{};
            memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

            VulkanBuffer stagingBuffer;
            device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 stagingBuffer, bufferSize);

            device->MapMemory(stagingBuffer);
            memcpy(stagingBuffer.mapped, buffer, bufferSize);
            device->unMapMemory(stagingBuffer);

            VkImageCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = format;
            imageCreateInfo.mipLevels = mipLevels;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCreateInfo.extent = {width, height, 1};
            imageCreateInfo.usage =
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            device->CreateImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, deviceMemory);

            VkCommandBuffer copyCmd = device->beginSingleTimeCommands();

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = 1;

            VkImageMemoryBarrier imageMemoryBarrier{};

            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                 nullptr, 0, nullptr, 1, &imageMemoryBarrier);

            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = width;
            bufferCopyRegion.imageExtent.height = height;
            bufferCopyRegion.imageExtent.depth = 1;

            vkCmdCopyBufferToImage(copyCmd, stagingBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                   &bufferCopyRegion);

            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                                 0, nullptr, 1, &imageMemoryBarrier);

            device->endSingleTimeCommands(copyCmd, copyQueue);
            device->destroyVulkanBuffer(stagingBuffer);

            // Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
            VkCommandBuffer blitCmd = device->beginSingleTimeCommands();
            for (uint32_t i = 1; i < mipLevels; i++) {
                VkImageBlit imageBlit{};

                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.layerCount = 1;
                imageBlit.srcSubresource.mipLevel = i - 1;
                imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
                imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
                imageBlit.srcOffsets[1].z = 1;

                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.layerCount = 1;
                imageBlit.dstSubresource.mipLevel = i;
                imageBlit.dstOffsets[1].x = int32_t(width >> i);
                imageBlit.dstOffsets[1].y = int32_t(height >> i);
                imageBlit.dstOffsets[1].z = 1;

                VkImageSubresourceRange mipSubRange = {};
                mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mipSubRange.baseMipLevel = i;
                mipSubRange.levelCount = 1;
                mipSubRange.layerCount = 1;

                {
                    VkImageMemoryBarrier imageMemoryBarrier{};
                    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageMemoryBarrier.srcAccessMask = 0;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageMemoryBarrier.image = image;
                    imageMemoryBarrier.subresourceRange = mipSubRange;
                    vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                         nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                }

                vkCmdBlitImage(blitCmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

                {
                    VkImageMemoryBarrier imageMemoryBarrier{};
                    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    imageMemoryBarrier.image = image;
                    imageMemoryBarrier.subresourceRange = mipSubRange;
                    vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                         nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                }
            }

            subresourceRange.levelCount = mipLevels;
            imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                 nullptr, 0, nullptr, 1, &imageMemoryBarrier);

            if (deleteBuffer) {
                delete[] buffer;
            }

            device->endSingleTimeCommands(blitCmd, copyQueue);
        } else {
            // Texture is stored in an external ktx file
            std::string filename = path + "/" + gltfimage.uri;

            ktxTexture *ktxTexture;

            ktxResult result = KTX_SUCCESS;
#if defined(__ANDROID__)
            AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
            if (!asset) {
                vks::tools::exitFatal("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
            }
            size_t size = AAsset_getLength(asset);
            assert(size > 0);
            ktx_uint8_t* textureData = new ktx_uint8_t[size];
            AAsset_read(asset, textureData, size);
            AAsset_close(asset);
            result = ktxTexture_CreateFromMemory(textureData, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
            delete[] textureData;
#else
            if (!fileExists(filename)) {
                exitFatal("Could not load texture from " + filename +
                          "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
            }
            result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                                    &ktxTexture);
#endif
            assert(result == KTX_SUCCESS);

            this->device = device;
            width = ktxTexture->baseWidth;
            height = ktxTexture->baseHeight;
            mipLevels = ktxTexture->numLevels;

            ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
            ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);
            // @todo: Use ktxTexture_GetVkFormat(ktxTexture)
            format = VK_FORMAT_R8G8B8A8_UNORM;

            // Get device properties for the requested texture format
            VkFormatProperties formatProperties;
            device->GetPhysicalDeviceFormatProperties(format, &formatProperties);

            VkCommandBuffer copyCmd = device->beginSingleTimeCommands();
            VulkanBuffer stagingBuffer;
            device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 stagingBuffer, ktxTextureSize);

            device->MapMemory(stagingBuffer);
            memcpy(stagingBuffer.mapped, ktxTextureData, ktxTextureSize);
            device->unMapMemory(stagingBuffer);

            std::vector<VkBufferImageCopy> bufferCopyRegions;
            for (uint32_t i = 0; i < mipLevels; i++) {
                ktx_size_t offset;
                KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
                assert(result == KTX_SUCCESS);
                VkBufferImageCopy bufferCopyRegion = {};
                bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel = i;
                bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.width = std::max(1u, ktxTexture->baseWidth >> i);
                bufferCopyRegion.imageExtent.height = std::max(1u, ktxTexture->baseHeight >> i);
                bufferCopyRegion.imageExtent.depth = 1;
                bufferCopyRegion.bufferOffset = offset;
                bufferCopyRegions.push_back(bufferCopyRegion);
            }

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
            imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            device->CreateImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, deviceMemory);

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = mipLevels;
            subresourceRange.layerCount = 1;

            device->transitionImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_UNDEFINED,
                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                          subresourceRange);
            vkCmdCopyBufferToImage(copyCmd, stagingBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());
            device->transitionImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
            device->endSingleTimeCommands(copyCmd, copyQueue);
            this->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            device->destroyVulkanBuffer(stagingBuffer);

            ktxTexture_Destroy(ktxTexture);
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.maxAnisotropy = 1.0;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxLod = (float) mipLevels;
        samplerInfo.maxAnisotropy = 8.0f;
        samplerInfo.anisotropyEnable = VK_TRUE;
        device->CreateSampler(&samplerInfo, &sampler);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.levelCount = mipLevels;
        device->CreateImageView(&viewInfo, &view);

        descriptor.sampler = sampler;
        descriptor.imageView = view;
        descriptor.imageLayout = imageLayout;
    }

/*
	glTF material
*/
    void
    Material::createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags) {
        device->CreateDescriptorSet(1, descriptorSetLayout, descriptorSet);
        std::vector<VkDescriptorImageInfo> imageDescriptors{};
        std::vector<VkWriteDescriptorSet> writeDescriptorSets{};
        if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
            imageDescriptors.push_back(baseColorTexture->descriptor);
            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.dstSet = descriptorSet;
            writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
            writeDescriptorSet.pImageInfo = &baseColorTexture->descriptor;
            writeDescriptorSets.push_back(writeDescriptorSet);
        }
        if (normalTexture && descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
            imageDescriptors.push_back(normalTexture->descriptor);
            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.dstSet = descriptorSet;
            writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
            writeDescriptorSet.pImageInfo = &normalTexture->descriptor;
            writeDescriptorSets.push_back(writeDescriptorSet);
        }
        device->updateDescriptorSets(writeDescriptorSets.data(), writeDescriptorSets.size());
    }


/*
	glTF primitive
*/
    void Primitive::setDimensions(glm::vec3 min, glm::vec3 max) {
        dimensions.min = min;
        dimensions.max = max;
        dimensions.size = max - min;
        dimensions.center = (min + max) / 2.0f;
        dimensions.radius = glm::distance(min, max) / 2.0f;
    }

/*
	glTF mesh
*/
    Mesh::Mesh(VulkanDevice *device, glm::mat4 matrix) {
        this->device = device;
        this->uniformBlock.matrix = matrix;
        device->CreateBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                uniformBuffer.buffer,
                sizeof(uniformBlock),
                &uniformBlock);
        device->MapMemory(uniformBuffer.buffer);
        uniformBuffer.descriptor = {uniformBuffer.buffer.buffer, 0, sizeof(uniformBlock)};
    };

    Mesh::~Mesh() {
        device->destroyVulkanBuffer(uniformBuffer.buffer);
        for (auto primitive: primitives) {
            delete primitive;
        }
    }

/*
	glTF node
*/
    glm::mat4 Node::localMatrix() {
        return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) *
               matrix;
    }

    glm::mat4 Node::getMatrix() {
        glm::mat4 m = localMatrix();
        Node *p = parent;
        while (p) {
            m = p->localMatrix() * m;
            p = p->parent;
        }
        return m;
    }

    void Node::update() {
        if (mesh) {
            glm::mat4 m = getMatrix();
            if (skin) {
                mesh->uniformBlock.matrix = m;
                // Update join matrices
                glm::mat4 inverseTransform = glm::inverse(m);
                for (size_t i = 0; i < skin->joints.size(); i++) {
                    Node *jointNode = skin->joints[i];
                    glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
                    jointMat = inverseTransform * jointMat;
                    mesh->uniformBlock.jointMatrix[i] = jointMat;
                }
                mesh->uniformBlock.jointcount = (float) skin->joints.size();
                memcpy(mesh->uniformBuffer.buffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
            } else {
                memcpy(mesh->uniformBuffer.buffer.mapped, &m, sizeof(glm::mat4));
            }
        }

        for (auto &child: children) {
            child->update();
        }
    }

    Node::~Node() {
        if (mesh) {
            delete mesh;
        }
        for (auto &child: children) {
            delete child;
        }
    }

/*
	glTF default vertex layout with easy Vulkan mapping functions
*/

    VkVertexInputBindingDescription Vertex::vertexInputBindingDescription;
    std::vector<VkVertexInputAttributeDescription> Vertex::vertexInputAttributeDescriptions;
    VkPipelineVertexInputStateCreateInfo Vertex::pipelineVertexInputStateCreateInfo;

    VkVertexInputBindingDescription Vertex::inputBindingDescription(uint32_t binding) {
        return VkVertexInputBindingDescription({binding, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
    }

    VkVertexInputAttributeDescription
    Vertex::inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component) {
        switch (component) {
            case VertexComponent::Position:
                return VkVertexInputAttributeDescription(
                        {location, binding, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>offsetof(Vertex, pos)});
            case VertexComponent::Normal:
                return VkVertexInputAttributeDescription(
                        {location, binding, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>offsetof(Vertex, normal)});
            case VertexComponent::UV:
                return VkVertexInputAttributeDescription(
                        {location, binding, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>offsetof(Vertex, uv)});
            case VertexComponent::Color:
                return VkVertexInputAttributeDescription(
                        {location, binding, VK_FORMAT_R32G32B32A32_SFLOAT,
                         static_cast<uint32_t>offsetof(Vertex, color)});
            case VertexComponent::Tangent:
                return VkVertexInputAttributeDescription(
                        {location, binding, VK_FORMAT_R32G32B32A32_SFLOAT,
                         static_cast<uint32_t>offsetof(Vertex, tangent)});
            case VertexComponent::Joint0:
                return VkVertexInputAttributeDescription(
                        {location, binding, VK_FORMAT_R32G32B32A32_SFLOAT,
                         static_cast<uint32_t>offsetof(Vertex, joint0)});
            case VertexComponent::Weight0:
                return VkVertexInputAttributeDescription(
                        {location, binding, VK_FORMAT_R32G32B32A32_SFLOAT,
                         static_cast<uint32_t>offsetof(Vertex, weight0)});
            default:
                return VkVertexInputAttributeDescription({});
        }
    }

    std::vector<VkVertexInputAttributeDescription>
    Vertex::inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> components) {
        std::vector<VkVertexInputAttributeDescription> result;
        uint32_t location = 0;
        for (VertexComponent component: components) {
            result.push_back(Vertex::inputAttributeDescription(binding, location, component));
            location++;
        }
        return result;
    }

/** @brief Returns the default pipeline vertex input state create info structure for the requested vertex components */
    VkPipelineVertexInputStateCreateInfo *
    Vertex::getPipelineVertexInputState(const std::vector<VertexComponent> components) {
        vertexInputBindingDescription = Vertex::inputBindingDescription(0);
        Vertex::vertexInputAttributeDescriptions = Vertex::inputAttributeDescriptions(0, components);
        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
        pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &Vertex::vertexInputBindingDescription;
        pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::vertexInputAttributeDescriptions.size());
        pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions.data();
        return &pipelineVertexInputStateCreateInfo;
    }

    Texture *Model::getTexture(uint32_t index) {

        if (index < textures.size()) {
            return &textures[index];
        }
        return nullptr;
    }

    void Model::createEmptyTexture(VkQueue transferQueue) {
        emptyTexture.device = device;
        emptyTexture.width = 1;
        emptyTexture.height = 1;
        emptyTexture.layerCount = 1;
        emptyTexture.mipLevels = 1;

        size_t bufferSize = emptyTexture.width * emptyTexture.height * 4;
        unsigned char *buffer = new unsigned char[bufferSize];
        memset(buffer, 0, bufferSize);

        VulkanBuffer stagingBuffer;
        device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, bufferSize);

        device->MapMemory(stagingBuffer);
        memcpy(stagingBuffer.mapped, buffer, bufferSize);
        device->unMapMemory(stagingBuffer);

        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = emptyTexture.width;
        bufferCopyRegion.imageExtent.height = emptyTexture.height;
        bufferCopyRegion.imageExtent.depth = 1;

        // Create optimal tiled target image
        VkImageCreateInfo imageCreateInfo = CreateImageCreateInfo();
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent = {emptyTexture.width, emptyTexture.height, 1};
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        device->CreateImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, emptyTexture.image,
                                    emptyTexture.deviceMemory);

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;

        VkCommandBuffer copyCmd = device->beginSingleTimeCommands();
        device->transitionImageLayout(copyCmd, emptyTexture.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
        vkCmdCopyBufferToImage(copyCmd, stagingBuffer.buffer, emptyTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &bufferCopyRegion);
        device->transitionImageLayout(copyCmd, emptyTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
        device->endSingleTimeCommands(copyCmd, transferQueue);
        emptyTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Clean up staging resources
        device->destroyVulkanBuffer(stagingBuffer);

        VkSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.maxAnisotropy = 1.0f;
        device->CreateSampler(&samplerCreateInfo, &emptyTexture.sampler);

        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.image = emptyTexture.image;
        device->CreateImageView(&viewCreateInfo, &emptyTexture.view);

        emptyTexture.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        emptyTexture.descriptor.imageView = emptyTexture.view;
        emptyTexture.descriptor.sampler = emptyTexture.sampler;
    }

/*
	glTF model loading and rendering class
*/
    Model::~Model() {
        device->destroyVulkanBuffer(vertices.buffer);
        device->destroyVulkanBuffer(indices.buffer);
        for (auto texture: textures) {
            texture.destroy();
        }
        for (auto node: nodes) {
            delete node;
        }
        for (auto skin: skins) {
            delete skin;
        }
        if (descriptorSetLayoutUbo != VK_NULL_HANDLE) {
            device->DestroyDescriptorSetLayout(descriptorSetLayoutUbo);
            descriptorSetLayoutUbo = VK_NULL_HANDLE;
        }
        if (descriptorSetLayoutImage != VK_NULL_HANDLE) {
            device->DestroyDescriptorSetLayout(descriptorSetLayoutImage);
            descriptorSetLayoutImage = VK_NULL_HANDLE;
        }
        emptyTexture.destroy();
    }

    void Model::loadNode(Node *parent, const tinygltf::Node &node, uint32_t nodeIndex,
                         const tinygltf::Model &model, std::vector<uint32_t> &indexBuffer,
                         std::vector<Vertex> &vertexBuffer, float globalscale) {
        Node *newNode = new Node{};
        newNode->index = nodeIndex;
        newNode->parent = parent;
        newNode->name = node.name;
        newNode->skinIndex = node.skin;
        newNode->matrix = glm::mat4(1.0f);

        // Generate local node matrix
        glm::vec3 translation = glm::vec3(0.0f);
        if (node.translation.size() == 3) {
            translation = glm::make_vec3(node.translation.data());
            newNode->translation = translation;
        }
        glm::mat4 rotation = glm::mat4(1.0f);
        if (node.rotation.size() == 4) {
            glm::quat q = glm::make_quat(node.rotation.data());
            newNode->rotation = glm::mat4(q);
        }
        glm::vec3 scale = glm::vec3(1.0f);
        if (node.scale.size() == 3) {
            scale = glm::make_vec3(node.scale.data());
            newNode->scale = scale;
        }
        if (node.matrix.size() == 16) {
            newNode->matrix = glm::make_mat4x4(node.matrix.data());
            if (globalscale != 1.0f) {
                //newNode->matrix = glm::scale(newNode->matrix, glm::vec3(globalscale));
            }
        };

        // Node with children
        if (node.children.size() > 0) {
            for (auto i = 0; i < node.children.size(); i++) {
                loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer,
                         globalscale);
            }
        }

        // Node contains mesh data
        if (node.mesh > -1) {
            const tinygltf::Mesh mesh = model.meshes[node.mesh];
            Mesh *newMesh = new Mesh(device, newNode->matrix);
            newMesh->name = mesh.name;
            for (size_t j = 0; j < mesh.primitives.size(); j++) {
                const tinygltf::Primitive &primitive = mesh.primitives[j];
                if (primitive.indices < 0) {
                    continue;
                }
                uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
                uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
                uint32_t indexCount = 0;
                uint32_t vertexCount = 0;
                glm::vec3 posMin{};
                glm::vec3 posMax{};
                bool hasSkin = false;
                // Vertices
                {
                    const float *bufferPos = nullptr;
                    const float *bufferNormals = nullptr;
                    const float *bufferTexCoords = nullptr;
                    const float *bufferColors = nullptr;
                    const float *bufferTangents = nullptr;
                    uint32_t numColorComponents;
                    const uint16_t *bufferJoints = nullptr;
                    const float *bufferWeights = nullptr;

                    // Position attribute is required
                    assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

                    const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.find(
                            "POSITION")->second];
                    const tinygltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
                    bufferPos = reinterpret_cast<const float *>(&(model.buffers[posView.buffer].data[
                            posAccessor.byteOffset + posView.byteOffset]));
                    posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
                    posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);

                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                        const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.find(
                                "NORMAL")->second];
                        const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
                        bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[
                                normAccessor.byteOffset + normView.byteOffset]));
                    }

                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find(
                                "TEXCOORD_0")->second];
                        const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                        bufferTexCoords = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[
                                uvAccessor.byteOffset + uvView.byteOffset]));
                    }

                    if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor &colorAccessor = model.accessors[primitive.attributes.find(
                                "COLOR_0")->second];
                        const tinygltf::BufferView &colorView = model.bufferViews[colorAccessor.bufferView];
                        // Color buffer are either of type vec3 or vec4
                        numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
                        bufferColors = reinterpret_cast<const float *>(&(model.buffers[colorView.buffer].data[
                                colorAccessor.byteOffset + colorView.byteOffset]));
                    }

                    if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
                        const tinygltf::Accessor &tangentAccessor = model.accessors[primitive.attributes.find(
                                "TANGENT")->second];
                        const tinygltf::BufferView &tangentView = model.bufferViews[tangentAccessor.bufferView];
                        bufferTangents = reinterpret_cast<const float *>(&(model.buffers[tangentView.buffer].data[
                                tangentAccessor.byteOffset + tangentView.byteOffset]));
                    }

                    // Skinning
                    // Joints
                    if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor &jointAccessor = model.accessors[primitive.attributes.find(
                                "JOINTS_0")->second];
                        const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];
                        bufferJoints = reinterpret_cast<const uint16_t *>(&(model.buffers[jointView.buffer].data[
                                jointAccessor.byteOffset + jointView.byteOffset]));
                    }

                    if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find(
                                "WEIGHTS_0")->second];
                        const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                        bufferWeights = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[
                                uvAccessor.byteOffset + uvView.byteOffset]));
                    }

                    hasSkin = (bufferJoints && bufferWeights);

                    vertexCount = static_cast<uint32_t>(posAccessor.count);

                    for (size_t v = 0; v < posAccessor.count; v++) {
                        Vertex vert{};
                        vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0f);
                        vert.normal = glm::normalize(
                                glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f)));
                        vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec3(0.0f);
                        if (bufferColors) {
                            switch (numColorComponents) {
                                case 3:
                                    vert.color = glm::vec4(glm::make_vec3(&bufferColors[v * 3]), 1.0f);
                                case 4:
                                    vert.color = glm::make_vec4(&bufferColors[v * 4]);
                            }
                        } else {
                            vert.color = glm::vec4(1.0f);
                        }
                        vert.tangent = bufferTangents ? glm::vec4(glm::make_vec4(&bufferTangents[v * 4])) : glm::vec4(
                                0.0f);
                        vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * 4])) : glm::vec4(0.0f);
                        vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * 4]) : glm::vec4(0.0f);
                        vertexBuffer.push_back(vert);
                    }
                }
                // Indices
                {
                    const tinygltf::Accessor &accessor = model.accessors[primitive.indices];
                    const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

                    indexCount = static_cast<uint32_t>(accessor.count);

                    switch (accessor.componentType) {
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                            uint32_t *buf = new uint32_t[accessor.count];
                            memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                   accessor.count * sizeof(uint32_t));
                            for (size_t index = 0; index < accessor.count; index++) {
                                indexBuffer.push_back(buf[index] + vertexStart);
                            }
                            delete[] buf;
                            break;
                        }
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                            uint16_t *buf = new uint16_t[accessor.count];
                            memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                   accessor.count * sizeof(uint16_t));
                            for (size_t index = 0; index < accessor.count; index++) {
                                indexBuffer.push_back(buf[index] + vertexStart);
                            }
                            delete[] buf;
                            break;
                        }
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                            uint8_t *buf = new uint8_t[accessor.count];
                            memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                   accessor.count * sizeof(uint8_t));
                            for (size_t index = 0; index < accessor.count; index++) {
                                indexBuffer.push_back(buf[index] + vertexStart);
                            }
                            delete[] buf;
                            break;
                        }
                        default:
                            std::cerr << "Index component type " << accessor.componentType << " not supported!"
                                      << std::endl;
                            return;
                    }
                }
                Primitive *newPrimitive = new Primitive(indexStart, indexCount,
                                                        primitive.material > -1 ? materials[primitive.material]
                                                                                : materials.back());
                newPrimitive->firstVertex = vertexStart;
                newPrimitive->vertexCount = vertexCount;
                newPrimitive->setDimensions(posMin, posMax);
                newMesh->primitives.push_back(newPrimitive);
            }
            newNode->mesh = newMesh;
        }
        if (parent) {
            parent->children.push_back(newNode);
        } else {
            nodes.push_back(newNode);
        }
        linearNodes.push_back(newNode);
    }

    void Model::loadSkins(tinygltf::Model &gltfModel) {
        for (tinygltf::Skin &source: gltfModel.skins) {
            Skin *newSkin = new Skin{};
            newSkin->name = source.name;

            // Find skeleton root node
            if (source.skeleton > -1) {
                newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
            }

            // Find joint nodes
            for (int jointIndex: source.joints) {
                Node *node = nodeFromIndex(jointIndex);
                if (node) {
                    newSkin->joints.push_back(nodeFromIndex(jointIndex));
                }
            }

            // Get inverse bind matrices from buffer
            if (source.inverseBindMatrices > -1) {
                const tinygltf::Accessor &accessor = gltfModel.accessors[source.inverseBindMatrices];
                const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];
                newSkin->inverseBindMatrices.resize(accessor.count);
                memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                       accessor.count * sizeof(glm::mat4));
            }

            skins.push_back(newSkin);
        }
    }

    void Model::loadImages(tinygltf::Model &gltfModel, VulkanDevice *device, VkQueue transferQueue) {
        for (tinygltf::Image &image: gltfModel.images) {
            Texture texture;
            texture.fromglTfImage(image, path, device, transferQueue);
            texture.index = static_cast<uint32_t>(textures.size());
            textures.push_back(texture);
        }
        // Create an empty texture to be used for empty material images
        createEmptyTexture(transferQueue);
    }

    void Model::loadMaterials(tinygltf::Model &gltfModel) {
        for (tinygltf::Material &mat: gltfModel.materials) {
            Material material(device);
            if (mat.values.find("baseColorTexture") != mat.values.end()) {
                material.baseColorTexture = getTexture(
                        gltfModel.textures[mat.values["baseColorTexture"].TextureIndex()].source);
            }
            // Metallic roughness workflow
            if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
                material.metallicRoughnessTexture = getTexture(
                        gltfModel.textures[mat.values["metallicRoughnessTexture"].TextureIndex()].source);
            }
            if (mat.values.find("roughnessFactor") != mat.values.end()) {
                material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
            }
            if (mat.values.find("metallicFactor") != mat.values.end()) {
                material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
            }
            if (mat.values.find("baseColorFactor") != mat.values.end()) {
                material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
            }
            if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
                material.normalTexture = getTexture(
                        gltfModel.textures[mat.additionalValues["normalTexture"].TextureIndex()].source);
            } else {
                material.normalTexture = &emptyTexture;
            }
            if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
                material.emissiveTexture = getTexture(
                        gltfModel.textures[mat.additionalValues["emissiveTexture"].TextureIndex()].source);
            }
            if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
                material.occlusionTexture = getTexture(
                        gltfModel.textures[mat.additionalValues["occlusionTexture"].TextureIndex()].source);
            }
            if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
                tinygltf::Parameter param = mat.additionalValues["alphaMode"];
                if (param.string_value == "BLEND") {
                    material.alphaMode = Material::ALPHAMODE_BLEND;
                }
                if (param.string_value == "MASK") {
                    material.alphaMode = Material::ALPHAMODE_MASK;
                }
            }
            if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
                material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
            }

            materials.push_back(material);
        }
        // Push a default material at the end of the list for meshes with no material assigned
        materials.push_back(Material(device));
    }

    void Model::loadAnimations(tinygltf::Model &gltfModel) {
        for (tinygltf::Animation &anim: gltfModel.animations) {
            Animation animation{};
            animation.name = anim.name;
            if (anim.name.empty()) {
                animation.name = std::to_string(animations.size());
            }

            // Samplers
            for (auto &samp: anim.samplers) {
                AnimationSampler sampler{};

                if (samp.interpolation == "LINEAR") {
                    sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
                }
                if (samp.interpolation == "STEP") {
                    sampler.interpolation = AnimationSampler::InterpolationType::STEP;
                }
                if (samp.interpolation == "CUBICSPLINE") {
                    sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
                }

                // Read sampler input time values
                {
                    const tinygltf::Accessor &accessor = gltfModel.accessors[samp.input];
                    const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

                    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                    float *buf = new float[accessor.count];
                    memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                           accessor.count * sizeof(float));
                    for (size_t index = 0; index < accessor.count; index++) {
                        sampler.inputs.push_back(buf[index]);
                    }
                    delete[] buf;
                    for (auto input: sampler.inputs) {
                        if (input < animation.start) {
                            animation.start = input;
                        };
                        if (input > animation.end) {
                            animation.end = input;
                        }
                    }
                }

                // Read sampler output T/R/S values
                {
                    const tinygltf::Accessor &accessor = gltfModel.accessors[samp.output];
                    const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

                    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                    switch (accessor.type) {
                        case TINYGLTF_TYPE_VEC3: {
                            glm::vec3 *buf = new glm::vec3[accessor.count];
                            memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                   accessor.count * sizeof(glm::vec3));
                            for (size_t index = 0; index < accessor.count; index++) {
                                sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
                            }
                            delete[] buf;
                            break;
                        }
                        case TINYGLTF_TYPE_VEC4: {
                            glm::vec4 *buf = new glm::vec4[accessor.count];
                            memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                   accessor.count * sizeof(glm::vec4));
                            for (size_t index = 0; index < accessor.count; index++) {
                                sampler.outputsVec4.push_back(buf[index]);
                            }
                            delete[] buf;
                            break;
                        }
                        default: {
                            std::cout << "unknown type" << std::endl;
                            break;
                        }
                    }
                }

                animation.samplers.push_back(sampler);
            }

            // Channels
            for (auto &source: anim.channels) {
                AnimationChannel channel{};

                if (source.target_path == "rotation") {
                    channel.path = AnimationChannel::PathType::ROTATION;
                }
                if (source.target_path == "translation") {
                    channel.path = AnimationChannel::PathType::TRANSLATION;
                }
                if (source.target_path == "scale") {
                    channel.path = AnimationChannel::PathType::SCALE;
                }
                if (source.target_path == "weights") {
                    std::cout << "weights not yet supported, skipping channel" << std::endl;
                    continue;
                }
                channel.samplerIndex = source.sampler;
                channel.node = nodeFromIndex(source.target_node);
                if (!channel.node) {
                    continue;
                }

                animation.channels.push_back(channel);
            }

            animations.push_back(animation);
        }
    }

    void Model::loadFromFile(std::string filename, VulkanDevice *device, VkQueue transferQueue,
                             uint32_t fileLoadingFlags, float scale) {
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF gltfContext;
        if (fileLoadingFlags & FileLoadingFlags::DontLoadImages) {
            gltfContext.SetImageLoader(loadImageDataFuncEmpty, nullptr);
        } else {
            gltfContext.SetImageLoader(loadImageDataFunc, nullptr);
        }
#if defined(__ANDROID__)
        // On Android all assets are packed with the apk in a compressed form, so we need to open them using the asset manager
        // We let tinygltf handle this, by passing the asset manager of our app
        tinygltf::asset_manager = androidApp->activity->assetManager;
#endif
        size_t pos = filename.find_last_of('/');
        path = filename.substr(0, pos);

        std::string error, warning;

        this->device = device;

#if defined(__ANDROID__)
        // On Android all assets are packed with the apk in a compressed form, so we need to open them using the asset manager
        // We let tinygltf handle this, by passing the asset manager of our app
        tinygltf::asset_manager = androidApp->activity->assetManager;
#endif
        bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename);

        std::vector<uint32_t> indexBuffer;
        std::vector<Vertex> vertexBuffer;

        if (fileLoaded) {
            if (!(fileLoadingFlags & FileLoadingFlags::DontLoadImages)) {
                loadImages(gltfModel, device, transferQueue);
            }
            loadMaterials(gltfModel);
            const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
            for (size_t i = 0; i < scene.nodes.size(); i++) {
                const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
                loadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);
            }
            if (gltfModel.animations.size() > 0) {
                loadAnimations(gltfModel);
            }
            loadSkins(gltfModel);

            for (auto node: linearNodes) {
                // Assign skins
                if (node->skinIndex > -1) {
                    node->skin = skins[node->skinIndex];
                }
                // Initial pose
                if (node->mesh) {
                    node->update();
                }
            }
        } else {
            // TODO: throw
            std::cerr << ("Could not load glTF file \"" + filename + "\": " + error) << std::endl;
            return;
        }

        // Pre-Calculations for requested features
        if ((fileLoadingFlags & FileLoadingFlags::PreTransformVertices) ||
            (fileLoadingFlags & FileLoadingFlags::PreMultiplyVertexColors) ||
            (fileLoadingFlags & FileLoadingFlags::FlipY)) {
            const bool preTransform = fileLoadingFlags & FileLoadingFlags::PreTransformVertices;
            const bool preMultiplyColor = fileLoadingFlags & FileLoadingFlags::PreMultiplyVertexColors;
            const bool flipY = fileLoadingFlags & FileLoadingFlags::FlipY;
            for (Node *node: linearNodes) {
                if (node->mesh) {
                    const glm::mat4 localMatrix = node->getMatrix();
                    for (Primitive *primitive: node->mesh->primitives) {
                        for (uint32_t i = 0; i < primitive->vertexCount; i++) {
                            Vertex &vertex = vertexBuffer[primitive->firstVertex + i];
                            // Pre-transform vertex positions by node-hierarchy
                            if (preTransform) {
                                vertex.pos = glm::vec3(localMatrix * glm::vec4(vertex.pos, 1.0f));
                                vertex.normal = glm::normalize(glm::mat3(localMatrix) * vertex.normal);
                            }
                            // Flip Y-Axis of vertex positions
                            if (flipY) {
                                vertex.pos.y *= -1.0f;
                                vertex.normal.y *= -1.0f;
                            }
                            // Pre-Multiply vertex colors with material base color
                            if (preMultiplyColor) {
                                vertex.color = primitive->material.baseColorFactor * vertex.color;
                            }
                        }
                    }
                }
            }
        }

        for (auto extension: gltfModel.extensionsUsed) {
            if (extension == "KHR_materials_pbrSpecularGlossiness") {
                std::cout << "Required extension: " << extension;
                metallicRoughnessWorkflow = false;
            }
        }

        size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
        size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
        indices.count = static_cast<uint32_t>(indexBuffer.size());
        vertices.count = static_cast<uint32_t>(vertexBuffer.size());

        assert((vertexBufferSize > 0) && (indexBufferSize > 0));

        VulkanBuffer vertexStaging, indexStaging;

        // Create staging buffers
        // Vertex data
        device->CreateBuffer(
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vertexBufferSize,
                &vertexStaging.buffer,
                &vertexStaging.memory,
                vertexBuffer.data());
        // Index data
        device->CreateBuffer(
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                indexBufferSize,
                &indexStaging.buffer,
                &indexStaging.memory,
                indexBuffer.data());

        // Create device local buffers
        // Vertex buffer
        device->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | memoryPropertyFlags,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             vertices.buffer,
                             vertexBufferSize);
        // Index buffer
        device->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | memoryPropertyFlags,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             indices.buffer,
                             indexBufferSize);

        // Copy from staging buffers
        VkCommandBuffer copyCmd = device->beginSingleTimeCommands();

        VkBufferCopy copyRegion = {};

        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer.buffer, 1, &copyRegion);

        copyRegion.size = indexBufferSize;
        vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer.buffer, 1, &copyRegion);

        device->endSingleTimeCommands(copyCmd, transferQueue);
        device->destroyVulkanBuffer(vertexStaging);
        device->destroyVulkanBuffer(indexStaging);

        getSceneDimensions();

        // Setup descriptors
        uint32_t uboCount{0};
        uint32_t imageCount{0};
        for (auto node: linearNodes) {
            if (node->mesh) {
                uboCount++;
            }
        }
        for (auto material: materials) {
            if (material.baseColorTexture != nullptr) {
                imageCount++;
            }
        }
        // Descriptors for per-node uniform buffers
        {
            // Layout is global, so only create if it hasn't already been created before
            if (descriptorSetLayoutUbo == VK_NULL_HANDLE) {
                std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                        CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                         VK_SHADER_STAGE_VERTEX_BIT, 0),
                };
                VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
                descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
                descriptorLayoutCI.pBindings = setLayoutBindings.data();
                device->CreateDescriptorSetLayout(&descriptorLayoutCI, &descriptorSetLayoutUbo);
            }
            for (auto node: nodes) {
                prepareNodeDescriptor(node, descriptorSetLayoutUbo);
            }
        }

        // Descriptors for per-material images
        {
            // Layout is global, so only create if it hasn't already been created before
            if (descriptorSetLayoutImage == VK_NULL_HANDLE) {
                std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
                if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
                    setLayoutBindings.push_back(
                            CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                             VK_SHADER_STAGE_FRAGMENT_BIT,
                                                             static_cast<uint32_t>(setLayoutBindings.size())));
                }
                if (descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
                    setLayoutBindings.push_back(
                            CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                             VK_SHADER_STAGE_FRAGMENT_BIT,
                                                             static_cast<uint32_t>(setLayoutBindings.size())));
                }
                VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
                descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
                descriptorLayoutCI.pBindings = setLayoutBindings.data();
                device->CreateDescriptorSetLayout(&descriptorLayoutCI, &descriptorSetLayoutImage);
            }
            for (auto &material: materials) {
                if (material.baseColorTexture != nullptr) {
                    material.createDescriptorSet(descriptorSetLayoutImage, descriptorBindingFlags);
                }
            }
        }
    }

    void Model::bindBuffers(VkCommandBuffer commandBuffer) {
        const VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer.buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indices.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        buffersBound = true;
    }

    void Model::drawNode(Node *node, VkCommandBuffer commandBuffer, uint32_t renderFlags,
                         VkPipelineLayout pipelineLayout, uint32_t bindImageSet) {
        if (node->mesh) {
            for (Primitive *primitive: node->mesh->primitives) {
                bool skip = false;
                const Material &material = primitive->material;
                if (renderFlags & RenderFlags::RenderOpaqueNodes) {
                    skip = (material.alphaMode != Material::ALPHAMODE_OPAQUE);
                }
                if (renderFlags & RenderFlags::RenderAlphaMaskedNodes) {
                    skip = (material.alphaMode != Material::ALPHAMODE_MASK);
                }
                if (renderFlags & RenderFlags::RenderAlphaBlendedNodes) {
                    skip = (material.alphaMode != Material::ALPHAMODE_BLEND);
                }
                if (!skip) {
                    if (renderFlags & RenderFlags::BindImages) {
                        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                                bindImageSet, 1, &material.descriptorSet, 0, nullptr);
                    }
                    vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
                }
            }
        }
        for (auto &child: node->children) {
            drawNode(child, commandBuffer, renderFlags, pipelineLayout, bindImageSet);
        }
    }

    void Model::draw(VkCommandBuffer commandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout,
                     uint32_t bindImageSet) {
        if (!buffersBound) {
            const VkDeviceSize offsets[1] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer.buffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indices.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        }
        for (auto &node: nodes) {
            drawNode(node, commandBuffer, renderFlags, pipelineLayout, bindImageSet);
        }
    }

    void Model::getNodeDimensions(Node *node, glm::vec3 &min, glm::vec3 &max) {
        if (node->mesh) {
            for (Primitive *primitive: node->mesh->primitives) {
                glm::vec4 locMin = glm::vec4(primitive->dimensions.min, 1.0f) * node->getMatrix();
                glm::vec4 locMax = glm::vec4(primitive->dimensions.max, 1.0f) * node->getMatrix();
                if (locMin.x < min.x) { min.x = locMin.x; }
                if (locMin.y < min.y) { min.y = locMin.y; }
                if (locMin.z < min.z) { min.z = locMin.z; }
                if (locMax.x > max.x) { max.x = locMax.x; }
                if (locMax.y > max.y) { max.y = locMax.y; }
                if (locMax.z > max.z) { max.z = locMax.z; }
            }
        }
        for (auto child: node->children) {
            getNodeDimensions(child, min, max);
        }
    }

    void Model::getSceneDimensions() {
        dimensions.min = glm::vec3(FLT_MAX);
        dimensions.max = glm::vec3(-FLT_MAX);
        for (auto node: nodes) {
            getNodeDimensions(node, dimensions.min, dimensions.max);
        }
        dimensions.size = dimensions.max - dimensions.min;
        dimensions.center = (dimensions.min + dimensions.max) / 2.0f;
        dimensions.radius = glm::distance(dimensions.min, dimensions.max) / 2.0f;
    }

    void Model::updateAnimation(uint32_t index, float time) {
        if (index > static_cast<uint32_t>(animations.size()) - 1) {
            std::cout << "No animation with index " << index << std::endl;
            return;
        }
        Animation &animation = animations[index];

        bool updated = false;
        for (auto &channel: animation.channels) {
            AnimationSampler &sampler = animation.samplers[channel.samplerIndex];
            if (sampler.inputs.size() > sampler.outputsVec4.size()) {
                continue;
            }

            for (auto i = 0; i < sampler.inputs.size() - 1; i++) {
                if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1])) {
                    float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
                    if (u <= 1.0f) {
                        switch (channel.path) {
                            case AnimationChannel::PathType::TRANSLATION: {
                                glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
                                channel.node->translation = glm::vec3(trans);
                                break;
                            }
                            case AnimationChannel::PathType::SCALE: {
                                glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
                                channel.node->scale = glm::vec3(trans);
                                break;
                            }
                            case AnimationChannel::PathType::ROTATION: {
                                glm::quat q1;
                                q1.x = sampler.outputsVec4[i].x;
                                q1.y = sampler.outputsVec4[i].y;
                                q1.z = sampler.outputsVec4[i].z;
                                q1.w = sampler.outputsVec4[i].w;
                                glm::quat q2;
                                q2.x = sampler.outputsVec4[i + 1].x;
                                q2.y = sampler.outputsVec4[i + 1].y;
                                q2.z = sampler.outputsVec4[i + 1].z;
                                q2.w = sampler.outputsVec4[i + 1].w;
                                channel.node->rotation = glm::normalize(glm::slerp(q1, q2, u));
                                break;
                            }
                        }
                        updated = true;
                    }
                }
            }
        }
        if (updated) {
            for (auto &node: nodes) {
                node->update();
            }
        }
    }

/*
	Helper functions
*/
    Node *Model::findNode(Node *parent, uint32_t index) {
        Node *nodeFound = nullptr;
        if (parent->index == index) {
            return parent;
        }
        for (auto &child: parent->children) {
            nodeFound = findNode(child, index);
            if (nodeFound) {
                break;
            }
        }
        return nodeFound;
    }

    Node *Model::nodeFromIndex(uint32_t index) {
        Node *nodeFound = nullptr;
        for (auto &node: nodes) {
            nodeFound = findNode(node, index);
            if (nodeFound) {
                break;
            }
        }
        return nodeFound;
    }

    void Model::prepareNodeDescriptor(Node *node, VkDescriptorSetLayout descriptorSetLayout) {
        if (node->mesh) {
            device->CreateDescriptorSet(1, descriptorSetLayout, node->mesh->uniformBuffer.descriptorSet);

            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.dstSet = node->mesh->uniformBuffer.descriptorSet;
            writeDescriptorSet.dstBinding = 0;
            writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.descriptor;

            device->updateDescriptorSets(&writeDescriptorSet);
        }
        for (auto &child: node->children) {
            prepareNodeDescriptor(child, descriptorSetLayout);
        }
    }
}
