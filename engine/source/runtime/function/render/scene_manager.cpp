#include "scene_manager.h"

namespace MW {

    void SceneManager::draw(VkCommandBuffer commandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout,
                            uint32_t bindImageSet, PushConstBlock *pushConstant, uint32_t pushSize) {
        for (int i = 0; i < models.size(); ++i) {
            auto &model = models[i];
            if (pushConstant) {
                pushConstant->position = modelPoss[i];
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, pushSize,
                                   pushConstant);
            }
            model->draw(commandBuffer, renderFlags, pipelineLayout, bindImageSet);
        }
    }

    void
    SceneManager::loadModel(const std::string &filename, VulkanDevice *device, uint32_t fileLoadingFlags,
                            glm::vec3 modelPos, float scale
    ) {
        models.emplace_back(std::make_shared<Model>());
        models.back()->loadFromFile(filename, device, fileLoadingFlags, scale);
        modelPoss.emplace_back(modelPos);
    }

    void SceneManager::initialize(SceneManagerInitInfo *initInfo) {
        device = initInfo->device;
        uint32_t glTFLoadingFlags = FileLoadingFlags::PreTransformVertices | FileLoadingFlags::FlipY;
        loadModel(getAssetPath() + "models/terrain_gridlines.gltf", device.get(), glTFLoadingFlags);

        const std::vector<glm::vec3> positions = {
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(1.25f, 0.25f, 1.25f),
                glm::vec3(-1.25f, -0.2f, 1.25f),
                glm::vec3(1.25f, 0.1f, -1.25f),
                glm::vec3(-1.25f, -0.25f, -1.25f),
        };
        for (auto &position: positions)
            loadModel(getAssetPath() + "models/oaktree.gltf", device.get(), glTFLoadingFlags, position);
    }
}