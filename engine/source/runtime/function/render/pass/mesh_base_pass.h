#pragma once

#include "runtime/function/render/pass/g_buffer_pass.h"
#include <array>

#if USE_MESH_SHADER
namespace MW {

    class MeshBaseBufferPass : public GBufferPass {
    public:
        virtual void draw() override;
    protected:
        virtual void createDescriptorSets() override;

        virtual void createPipelines() override;
    };
}
#endif