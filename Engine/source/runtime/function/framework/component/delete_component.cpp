#include "delete_component.h"
#include "function/framework/game_object.h"
#include "mesh_component.h"
#include "render_component.h"
#include "particle_component.h"

namespace MW {
    void DeleteComponent::tick() {
        auto mesh = m_parent_object.lock()->tryGetComponent(MeshComponent);
        if (time == MAX_TIME) {
            m_parent_object.lock()->tryGetComponent(RenderComponent)->setDirty(true);
            mesh->setDirty(false);
            m_parent_object.lock()->setShouldTick(false);
            finished = true;
            return;
        }
        mesh->setDirty(true);
        //将左下的点复制一个出来变为五边形
        if (time == 0) {
            mesh->points.emplace_back(mesh->points.back());
        }
        auto particleCom = m_parent_object.lock()->tryGetComponent(ParticleComponent);
        //后半部分删除时，三角形中其实也保存了五个点，铺粒子
        auto direction = mesh->points[4] - mesh->points[3];
        auto now = mesh->points[3];
        //使得将粒子数量翻倍，使得粒子更为稠密以达到抗锯齿的效果
        const int count = time * 2;
        for (int i = 0; i <= count; i++) {
            particleCom->addParticleEmitter(now);
            now += direction / static_cast<float>(count);
        }
        float halfTime = static_cast<float>(MAX_TIME) / 2;
        if (time < MAX_TIME / 2) {
            mesh->points[3].y -= height / halfTime;
            mesh->points[4].x += width / halfTime;
        } else {
            mesh->points[4].y -= height / halfTime;
            mesh->points[3].x += width / halfTime;
            mesh->points[2].y -= height / halfTime;
            mesh->points[1].x += width / halfTime;
        }
//        auto transform = m_parent_object.lock()->tryGetComponent(TransformComponent);
//        transform->scale -= SCALE / FRAME_TIME;
        ++time;
    }
}