#pragma once

//std
#include <memory>
#include <vector>

namespace MW {
    class MWEngine;
    class MWEditor {
    public:
        MWEditor() {};

        ~MWEditor() {};

        MWEditor(const MWEditor&) = delete;

        MWEditor& operator=(const MWEditor&) = delete;

        void run();

        void clear();
        void initialize();
    private:
        std::shared_ptr<MWEngine> engine;
    };
}