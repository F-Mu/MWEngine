#pragma once

#include "runtime/function/render/pass/pass_base.h"
#include "function/render/render_model.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "glm/glm.hpp"

namespace MW {
    class VulkanTextureCubeMap;

    struct CubePushBlockBuffer {
        struct CubePushBlock {
            glm::mat4 mvp{};
            void *data{nullptr};
        } pushBlock;
        uint32_t size{0};
    };

    struct CubePassInitInfo : public RenderPassInitInfo {
        std::vector<unsigned char> *fragShader{nullptr};
        bool useEnvironmentCube{true};
        CubePushBlockBuffer pushBlock;
        int32_t imageDim{64};

        explicit CubePassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };

    // execute only once for precompute
    class CubePass : public PassBase {
    public:
        void initialize(const RenderPassInitInfo *info) override;

    private:
        void draw() override;

        void createRenderPass();

        void createDescriptorSets();

        void createFramebuffers();

        void createPipelines();

        void createCubeMap();

        void loadCube();

        CubePushBlockBuffer pushBlockBuffer;
        bool useEnvironmentCube{true};
        std::vector<unsigned char> *fragShader{nullptr};
        int32_t imageDim{64};
        VulkanTextureCubeMap cubeMap;
        bool executed{false};
        float numMips;
        Model skybox;
    };
}