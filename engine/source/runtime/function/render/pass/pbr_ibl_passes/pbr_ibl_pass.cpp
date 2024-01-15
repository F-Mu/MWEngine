#include "pbr_ibl_pass.h"
#include "lut_pass.h"
#include "cube_pass.h"

namespace MW {
    void PbrIblPass::initialize(const RenderPassInitInfo *info) {
        PassBase::initialize(info);

        prefilteredPass = std::make_shared<CubePass>();
        irradiancePass = std::make_shared<CubePass>();
        lutPass = std::make_shared<LutPass>();
        prefilteredPass->initialize(info);
        irradiancePass->initialize(info);
        lutPass->initialize(info);
        precompute();
    }

    void PbrIblPass::preparePassData() {
        PassBase::preparePassData();
    }

    void PbrIblPass::precompute() {
        prefilteredPass->draw();
        irradiancePass->draw();
        lutPass->draw();
    }

    void PbrIblPass::draw() {
        PassBase::draw();
    }

    void PbrIblPass::createUniformBuffer() {

    }

    void PbrIblPass::createDescriptorSets() {

    }

    void PbrIblPass::createPipelines() {

    }
}