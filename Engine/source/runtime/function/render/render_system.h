#pragma once

//std
#include <cassert>
#include <memory>
#include <vector>

namespace MW {
    class WindowSystem;
    class VulkanDevice;

    struct RenderSystemInitInfo
    {
        std::shared_ptr<WindowSystem> window;
    };
    class RenderSystem {
    public:
        RenderSystem(RenderSystemInitInfo info);

        ~RenderSystem();

        RenderSystem(const RenderSystem&) = delete;

        RenderSystem& operator=(const RenderSystem&) = delete;

        //VkRenderPass getSwapChainRenderPass() const { return renderSwapChain->getRenderPass(); }

        //float getAspectRatio() const { return renderSwapChain->extentAspectRatio(); }

        /*bool isFrameInProgress() const {
            return isFrameStarted;
        }

        VkCommandBuffer getCurrentCommandBuffer() const {
            assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
            return commandBuffers[currentFrameIndex];
        }

        int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame not in progress");
            return currentFrameIndex;
        }

        VkCommandBuffer beginFrame();

        void endFrame();

        void beginSwapChainRenderPass();

        void endSwapChainRenderPass();

        std::shared_ptr<VkCommandBuffer> nowCommandBuffer;*/

    private:
        /*void createCommandBuffers();

        void freeCommandBuffers();

        void recreateSwapChain();

        std::unique_ptr<RenderSwapChain> renderSwapChain{};*/
        //std::vector<VkCommandBuffer> commandBuffers{};
        std::shared_ptr<VulkanDevice> vulkanDevice;
        /*std::shared_ptr<WindowSystem> windowSystem;
        uint32_t currentImageIndex{};
        int currentFrameIndex{};
        bool isFrameStarted{};*/
    };
}