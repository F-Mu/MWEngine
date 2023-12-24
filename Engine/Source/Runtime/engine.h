#pragma once


namespace MW
{
    class MWEngine
    {
        friend class MWEditor;

    public:
        MWEngine() {};
        ~MWEngine() {};
        void run();

        void tick(float deltaTime);

        void clear();
        void initialize();
        MWEngine(const MWEngine&) = delete;

        MWEngine& operator=(const MWEngine&) = delete;
    };
}