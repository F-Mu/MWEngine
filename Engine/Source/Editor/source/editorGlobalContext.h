#pragma once
#include <memory>
namespace MW
{
    class MWEngine;
    class EditorGlobalContext
    {
    public:
        std::shared_ptr<MWEngine> engine;
    public:
        void initialize();
        void clear();
    };
    extern EditorGlobalContext editorGlobalContext;

}