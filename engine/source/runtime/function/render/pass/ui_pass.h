#pragma once

#include "pass_base.h"

namespace MW {
    class WindowUI;

    class UIPass : public PassBase {
    public:
        void initialize(const RenderPassInitInfo *info) override;

    private:
        void uploadFonts();
        void createRenderPass();
        void createPipelines();
        void createDescriptorSets();
        void initImGui();
    private:
        WindowUI *m_window_ui;
    };
} // namespace Piccolo
