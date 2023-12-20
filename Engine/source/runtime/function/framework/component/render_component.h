#pragma once

#include <memory>
#include <glm/glm.h>
#include "render/render_device.h"
#include "render/render_buffer.h"
#include "component.h"
#include "core/vertex.h"
#include "render/render_model.h"

namespace MW {
    class RenderComponent : public Component {
    public:
        explicit RenderComponent(const std::weak_ptr<GameObject> &parent,const std::string &type);

        ~RenderComponent();

        void update();

        void tick() override;
    private:
        std::shared_ptr<Model> model{};
    };
}