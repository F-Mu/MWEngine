#pragma once

#include <memory>
namespace MW
{
    class MWEngine;
    class EngineGlobalContext
    {
    public:

    public:
        void initialize();
        void clear();
    };

    extern EngineGlobalContext engineGlobalContext;
}