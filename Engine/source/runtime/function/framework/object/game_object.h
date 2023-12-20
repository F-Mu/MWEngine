#pragma once

#include "component/component.h"
#include "component/transform_component.h"
#include "component/move_component.h"
//libs
#include <glm/gtc/matrix_transform.h>
//std
#include <memory>
#include <unordered_map>
#include <list>
#include <iostream>

namespace MW {
    class GameObject : public std::enable_shared_from_this<GameObject> {
    public:
        static GameObject createGameObject() {
            static id_t currentId = 0;
            return {currentId++};
        }

        virtual ~GameObject();

        GameObject(const GameObject &) = default;

        GameObject &operator=(const GameObject &) = default;

        GameObject(GameObject &&) = default;

        GameObject &operator=(GameObject &&) = default;

        template<typename TComponent>
        TComponent *tryGetComponent(const std::string &component_type_name) {
            for (auto &component: components) {
                if (component->getTypeName() == component_type_name) {
                    return static_cast<TComponent *>(component.operator->());
                }
            }
            return nullptr;
        }

        template<typename TComponent>
        const TComponent *tryGetComponentConst(const std::string &component_type_name) const {
            for (const auto &component: components) {
                if (component->getTypeName() == component_type_name) {
                    return static_cast<const TComponent *>(component.operator->());
                }
            }
            return nullptr;
        }

        //《Effective Modern C++》条款21:优先选用make系列，而非直接使用new
        // 去掉new而使用make_share，将可变参数变为万能引用，并加上完美转发
        template<typename TComponent, typename ...Args>
        void addComponent(const std::string &type, Args &&...args) {
            components.emplace_back(std::make_shared<TComponent>(weak_from_this(), type, std::forward<Args>(args)...));
        }

        void tick();

        id_t getId() const { return id; }

        std::list<std::shared_ptr<Component>> components;

        void setShouldTick(bool cond) { shouldTick = cond; }

#define tryGetComponent(COMPONENT_TYPE) tryGetComponent<COMPONENT_TYPE>(#COMPONENT_TYPE)
#define tryGetComponentConst(COMPONENT_TYPE) tryGetComponentConst<const COMPONENT_TYPE>(#COMPONENT_TYPE)
#define addComponent(COMPONENT_TYPE, ...) addComponent<COMPONENT_TYPE>(#COMPONENT_TYPE,##__VA_ARGS__)
    private:
        GameObject(id_t objId) : id{objId} {}

        id_t id;

        bool shouldTick{true};
    };
}