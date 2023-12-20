#pragma once

#include "render_device.h"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace MW {

    class RenderDescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder(RenderDevice &device) : renderDevice{device} {}

            Builder &addBinding(
                    uint32_t binding,
                    VkDescriptorType descriptorType,
                    VkShaderStageFlags stageFlags,
                    uint32_t count = 1);
            std::unique_ptr<RenderDescriptorSetLayout> build() const;

        private:
            RenderDevice &renderDevice;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
        };

        RenderDescriptorSetLayout(
                RenderDevice &device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~RenderDescriptorSetLayout();
        RenderDescriptorSetLayout(const RenderDescriptorSetLayout &) = delete;
        RenderDescriptorSetLayout &operator=(const RenderDescriptorSetLayout &) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        RenderDevice &renderDevice;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class RenderDescriptorWriter;
    };

    class RenderDescriptorPool {
    public:
        class Builder {
        public:
            Builder(RenderDevice &crpDevice) : renderDevice{crpDevice} {}

            Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder &setMaxSets(uint32_t count);
            std::unique_ptr<RenderDescriptorPool> build() const;

        private:
            RenderDevice &renderDevice;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        RenderDescriptorPool(
                RenderDevice &device,
                uint32_t maxSets,
                VkDescriptorPoolCreateFlags poolFlags,
                const std::vector<VkDescriptorPoolSize> &poolSizes);
        ~RenderDescriptorPool();
        RenderDescriptorPool(const RenderDescriptorPool &) = delete;
        RenderDescriptorPool &operator=(const RenderDescriptorPool &) = delete;

        bool allocateDescriptor(
                const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;

        void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

        void resetPool();

    private:
        RenderDevice &renderDevice;
        VkDescriptorPool descriptorPool;

        friend class RenderDescriptorWriter;
    };

    class RenderDescriptorWriter {
    public:
        RenderDescriptorWriter(RenderDescriptorSetLayout &setLayout, RenderDescriptorPool &pool);

        RenderDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
        RenderDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);

        bool build(VkDescriptorSet &set);
        void overwrite(VkDescriptorSet &set);

    private:
        RenderDescriptorSetLayout &setLayout;
        RenderDescriptorPool &pool;
        std::vector<VkWriteDescriptorSet> writes;
    };

}  // namespace MW