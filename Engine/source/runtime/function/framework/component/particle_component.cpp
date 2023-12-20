#include "particle_component.hpp"
#include "render/pass/particle_render_pass.hpp"
#include "function/global/global_context.hpp"

namespace MW {

    void ParticleComponent::addParticleEmitter(const glm::vec3 &position) {
        auto transform = m_parent_object.lock()->tryGetComponent(TransformComponent);
        auto pos = transform->mat4() * glm::vec4{position, 1.0};
        pos.z = PARTICLE_LAYER;
        emitters.emplace_back(pos);
    }

    void ParticleComponent::update() {
        for (auto &emitter: emitters)
            emitter.logicTick();
    }

    void ParticleComponent::tick() {
        update();
        for (auto &emitter: emitters)
            emitter.renderTick();
    }

    void ParticleEmitter::renderTick() {
        for (auto &particle: particles)
            particle.renderTick();
    }

    void Particle::renderTick() {
        if (life == 0)return;
        ParticlePushConstants push{};
        push.position = position;
        color[1] *= (static_cast<float>(getLife()) / MAX_LIFE);//使得颜色从高温黄，变为低温红，逐渐降低G值占比
        push.color = {color, 1.0};
        globalContext.particleRenderPass->bind(push);
        --life;
    }
}