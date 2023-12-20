#pragma once

#include <glm/glm.h>
#include <vector>
#include <list>
#include "core/macro.h"
#include "component.h"

namespace MW {
    class Particle {
    public:
        explicit Particle(const glm::vec4 &position) : position{position} {}

        void logicTick() {
            position += velocity;
        }

        void renderTick();

        [[nodiscard]] int getLife() const { return life; }

        constexpr static int MAX_LIFE = 7;
    private:
        int life{MAX_LIFE};
        glm::vec4 position;
        glm::vec3 color{1., .8, .0};
        glm::vec4 velocity{0, -0, 0, 0};
    };

    class ParticleEmitter {
    public:
        explicit ParticleEmitter(const glm::vec4 &position) : position{position} {
            particles.emplace_back(position);
        }

        void logicTick() {
            for (auto &particle: particles) {
                particle.logicTick();
            }
        }

        void renderTick();

        glm::vec4 position;
        std::list<Particle> particles;
    };

    class ParticleComponent : public Component {
    public:
        explicit ParticleComponent(const std::weak_ptr<GameObject> &parent, const std::string &type) :
                Component(parent, type) {}

        void addParticleEmitter(const glm::vec3 &position);

        void update();

        void tick() override;

    private:
        std::list<ParticleEmitter> emitters;
    };
}