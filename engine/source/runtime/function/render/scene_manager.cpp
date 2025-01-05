#include "scene_manager.h"

namespace MW {

    void SceneManager::draw(VkCommandBuffer commandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout,
                            uint32_t bindImageSet, PushConstBlock *pushConstant, uint32_t pushSize, bool bUseMeshShader) {
        for (int i = 0; i < models.size(); ++i) {
            auto &model = models[i];
            if (!bUseMeshShader) { // 不使用mesh shader情况下直接这里绑定
                pushConstant->position = modelPoss[i];
                vkCmdPushConstants(commandBuffer, pipelineLayout,
                                   bUseMeshShader ? VK_SHADER_STAGE_MESH_BIT_NV : VK_SHADER_STAGE_VERTEX_BIT, 0,
                                   pushSize,
                                   pushConstant);
            }
#if USE_MESH_SHADER
            model->bUseMeshShader = bUseMeshShader;
            model->setVertexDescriptorFirstSet(2);
            model->setMeshletDescriptorFirstSet(3);
#endif
            model->draw(commandBuffer, renderFlags, pipelineLayout, bindImageSet);
        }
    }

    void
    SceneManager::loadModel(const std::string &filename, uint32_t fileLoadingFlags,
                            glm::vec3 modelPos, float scale
    ) {
#if USE_MESH_SHADER
        models.emplace_back(std::make_shared<Model>(true));
#else
        models.emplace_back(std::make_shared<Model>());
#endif
        auto&model =models.back();
        model->loadFromFile(filename, device.get(), fileLoadingFlags, scale);
        for(auto&node:model->nodes){
            for(auto&primitive:node->mesh->primitives){
                primitive->pushConstantBlock.position = modelPos; // todo: 改成model矩阵
            }
        }
        modelPoss.emplace_back(modelPos);
    }

    void SceneManager::initialize(SceneManagerInitInfo *initInfo) {
        device = initInfo->device;
        uint32_t glTFLoadingFlags = FileLoadingFlags::PreTransformVertices | FileLoadingFlags::FlipY;
        loadModel(getAssetPath() + "models/sponza/sponza.gltf", glTFLoadingFlags);
//        loadModel(getAssetPath() + "models/cube.gltf", glTFLoadingFlags);
        skybox = std::make_shared<VulkanTextureCubeMap>();
        skybox->loadFromFile(getAssetPath() + "textures/hdr/pisa_cube.ktx",
                             VK_FORMAT_R16G16B16A16_SFLOAT, device);
//        const std::vector<glm::vec3> positions = {
//                glm::vec3(0.0f, 0.0f, 0.0f),
//                glm::vec3(1.25f, 0.25f, 1.25f),
//                glm::vec3(-1.25f, -0.2f, 1.25f),
//                glm::vec3(1.25f, 0.1f, -1.25f),
//                glm::vec3(-1.25f, -0.25f, -1.25f),
//        };
//        for (auto &position: positions)
//            loadModel(getAssetPath() + "models/oaktree.gltf", device.get(), glTFLoadingFlags, position);
    }
    void SceneManager::clean(){
        skybox->destroy(device);
        for(auto&model:models)
            model->clean();
    }
}