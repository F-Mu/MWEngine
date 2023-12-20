#pragma once
#include <memory>
namespace MW
{
    class MWEngine;
    class WindowSystem;
    class RenderSystem;
    class EditorGlobalContext
    {
    public:
        std::shared_ptr<WindowSystem> windowSystem;
        std::shared_ptr<RenderSystem> renderSystem;
    public:
        void initialize();
        void clear();
    };
    extern EditorGlobalContext editorGlobalContext;

}