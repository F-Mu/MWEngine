#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "component.h"
#include "core/vertex.h"
#include "function/render/render_mesh.h"
namespace MW {
    class RenderComponent : public Component {
    public:
        explicit RenderComponent(const std::weak_ptr<GameObject> &parent,const std::string &type):
            Component(parent, type) {}

        ~RenderComponent();

        void update();

        void tick() override;
    private:
        std::shared_ptr<RenderMesh>mesh;
    };
}