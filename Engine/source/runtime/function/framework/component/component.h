#pragma once

#include <string>
#include <utility>

namespace MW {
    class GameObject;

    class Component {
    public:
        explicit Component(std::weak_ptr<GameObject> parent, std::string type) : m_parent_object{std::move(parent)},
                                                                                 type{std::move(type)} {};

        std::weak_ptr<GameObject> m_parent_object;

        virtual void tick() {};

        virtual ~Component() = default;

        std::string getTypeName() { return type; }

        [[nodiscard]] bool isDirty() const { return is_dirty; }

        void setDirty(bool cond) { is_dirty = cond; }

    private:
        bool is_dirty{false};
    protected:
        std::string type;
    };
}