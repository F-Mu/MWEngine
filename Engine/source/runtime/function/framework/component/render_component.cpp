#include "render_component.hpp"
#include "function/global/global_context.hpp"
#include "function/framework/game_object.hpp"
#include "mesh_component.hpp"
#include "delete_component.hpp"
#include "render/render_model.hpp"

namespace MW {
    RenderComponent::RenderComponent(const std::weak_ptr<GameObject> &parent, const std::string &type) :
            Component(parent, type) {
        auto meshComponent = parent.lock()->tryGetComponentConst(MeshComponent);
        if (!meshComponent)return;
        if (meshComponent->vertexPoints.empty())return;
        if (meshComponent->worldIndices.empty())return;
        model = std::make_shared<Model>(meshComponent->vertexPoints, meshComponent->worldIndices);
    }

    RenderComponent::~RenderComponent() {
        model.reset();
    }

    void RenderComponent::update() {
        auto deleteComponent = m_parent_object.lock()->tryGetComponentConst(DeleteComponent);
        if (deleteComponent && deleteComponent->isFinished()) {
            model = nullptr;
            return;
        }
        if (!model)return;
        auto meshComponent = m_parent_object.lock()->tryGetComponentConst(MeshComponent);
        if (!meshComponent || !meshComponent->isDirty())return;
        if (meshComponent->vertexPoints.empty())return;
        model = std::make_shared<Model>(meshComponent->vertexPoints);
    }

    void RenderComponent::tick() {
//        if (!model || isDirty())return;
        update();
//        if(!model)std::cout<<"!";
        if (!model)return;
        auto transform = m_parent_object.lock()->tryGetComponent(TransformComponent);
        PushConstantData push{};
        push.modelMatrix = transform->mat4();
        push.normalMatrix = transform->normalMatrix();
        globalContext.simpleRenderPass->bind(push, model);
    }
}