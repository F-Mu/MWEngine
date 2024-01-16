#pragma once

#include "runtime/function/render/pass/pass_base.h"
#include "function/render/render_model.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "glm/glm.hpp"

namespace MW {
    class VulkanTextureCubeMap;

    struct prefilterPushBlock {
        float roughness;
        uint32_t numSamples = 32u;
    };
    struct irradiancePushBlock {
        float deltaPhi = 2.0f * float(M_PI) / 180.0f;
        float deltaTheta = 0.5f * float(M_PI) / 64.0f;
    };
    struct CubePushBlockBuffer {
        struct CubePushBlock {
            glm::mat4 mvp{};
            void *data{nullptr};
        } pushBlock;
        uint32_t size{0};
    };
    enum CubePassType {
        prefilter_cube_pass,
        irradiance_cube_pass
    };
    struct CubePassInitInfo : public RenderPassInitInfo {
        const std::vector<unsigned char> *fragShader{nullptr};
        bool useEnvironmentCube{true};
        CubePushBlockBuffer pushBlock;
        int32_t imageDim{64};
        CubePassType type;

        explicit CubePassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };

    // execute only once for precompute
    class CubePass : public PassBase {
    public:
        void initialize(const RenderPassInitInfo *info) override;

        void draw() override;
        VulkanTextureCubeMap cubeMap;

    private:
        void updatePushBlock(uint32_t mip);

        void createRenderPass();

        void createDescriptorSets();

        void createFramebuffers();

        void createPipelines();

        void createCubeMap();

        void loadCube();

        static constexpr VkFormat cubeMapFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        CubePushBlockBuffer pushBlockBuffer;
        bool useEnvironmentCube{true};
        const std::vector<unsigned char> *fragShader{nullptr};
        int32_t imageDim{64};
        bool executed{false};
        uint32_t numMips;
        Model cube;
        CubePassType type;
    };
}