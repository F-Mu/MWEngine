#pragma once

#include "render_model.h"
#include <glm/glm.hpp>
#include <vector>

namespace MW {
    class SceneManager {
    public:
        void
        loadModel(const std::string &filename, VulkanDevice *device, uint32_t fileLoadingFlags = FileLoadingFlags::None,
                  float scale = 1.0, glm::vec3 modelPos = glm::vec3(0.0f));

        void
        draw(VkCommandBuffer commandBuffer, uint32_t renderFlags = 0, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE,
             uint32_t bindImageSet = 1, void *pushConstant = nullptr, uint32_t pushSize = 0);

        std::vector<Model> models;
        std::vector<glm::vec3> modelPoss;
    };
}