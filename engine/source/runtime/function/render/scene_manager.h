#pragma once

#include "render_model.h"
#include <vector>

namespace MW {
    struct SceneManagerInitInfo {
        std::shared_ptr<VulkanDevice> device;
    };

    struct PushConstBlock {
        glm::vec3 position{0};
    };

    class SceneManager {
    public:
        void
        loadModel(const std::string &filename,uint32_t fileLoadingFlags = FileLoadingFlags::None,
                  glm::vec3 modelPos = glm::vec3(0.0f), float scale = 1.0);

        void
        draw(VkCommandBuffer commandBuffer, uint32_t renderFlags = 0, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE,
             uint32_t bindImageSet = 1, PushConstBlock *pushConstant = nullptr, uint32_t pushSize = 0);

        void initialize(SceneManagerInitInfo *initInfo);

        std::shared_ptr<VulkanTextureCubeMap> getSkyBox() { return skybox; }

    private:
        std::shared_ptr<VulkanDevice> device;
        std::vector<std::shared_ptr<Model>> models; /* 存指针！！！不然扩容时会全析构 */
        std::vector<glm::vec3> modelPoss;
        std::shared_ptr<VulkanTextureCubeMap> skybox;
    };
}