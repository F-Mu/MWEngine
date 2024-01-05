#include "scene_manager.h"

namespace MW {

    void SceneManager::draw(VkCommandBuffer commandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout,
                            uint32_t bindImageSet, void *pushConstant, uint32_t pushSize) {
        for (auto &model: models) {
            if (pushConstant)
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, pushSize,
                                   pushConstant);
            model.draw(commandBuffer, renderFlags, pipelineLayout, bindImageSet);
        }
    }

    void
    SceneManager::loadModel(const std::string &filename, VulkanDevice *device, uint32_t fileLoadingFlags, float scale,
                            glm::vec3 modelPos) {
        Model model;
        model.loadFromFile(filename, device, fileLoadingFlags, scale);
        modelPoss.emplace_back(modelPos);
        models.emplace_back(model);
    }
}