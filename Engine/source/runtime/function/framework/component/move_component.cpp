#include "move_component.h"
#include "transform_component.h"
#include "function/framework/game_object.h"

namespace MW {
    void MoveComponent::tick() {
        if (isFinished())return;
        MoveToPoint(destinations[now]);
        if (FIX_EQUAL(center, destinations[now])) {
            ++now;
            if (isFinished())return;
            auto dir = destinations[now] - destinations[now - 1];
            direction = glm::normalize(dir) * speed;
        }
    }

    MoveComponent::MoveComponent(const std::weak_ptr<GameObject> &parent, const std::string &type)
            : Component(parent, type),
              center{m_parent_object.lock()->tryGetComponent(TransformComponent)->translation} {
    }

    void MoveComponent::MoveToPoint(glm::vec3 &destination) {
        direction[2] = 0;
        int flag, pos, flag2;
        if (fabs(center[0] - destination[0]) < EPS)flag = center[1] - destination[1] > 0 ? 1 : -1, pos = 1;
        else flag = center[0] - destination[0] > 0 ? 1 : -1, pos = 0;
        if (FIX_EQUAL(center, destination)) {
            return;
        }
        center += direction;
        flag2 = center[pos] - destination[pos] > 0 ? 1 : -1;
        if (flag != flag2) {
            center[0] = destination[0];
            center[1] = destination[1];
        }
        if (FIX_EQUAL(center, destination)) {
            return;
        }
    }

    void MoveComponent::setDestination(glm::vec3 &_destination, float _time) {
        if (!isFinished())return;
        destinations.emplace_back(_destination);
        auto dir = destinations[0] - center;
        direction = dir / _time;
    }

    void MoveComponent::setDestination(std::vector<glm::vec3> &_destinations, float _time) {
        if (!isFinished())return;
        destinations = _destinations;
        auto dir = destinations[0] - center;
        float len = glm::length(dir);
        for (int i = 1; i < destinations.size(); ++i) {
            auto d = destinations[i] - destinations[i - 1];
            len += glm::length(d);
        }
        speed = len / _time;
        direction = glm::normalize(dir) * speed;
    }
}