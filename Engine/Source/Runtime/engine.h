#pragma once


namespace MW
{
    class MWEngine
    {
        friend class PiccoloEditor;

    public:
        MWEngine() {};
        ~MWEngine() {};
        void run();

        void Tick(float deltaTime);

        void clear();
        void initialize();
        MWEngine(const MWEngine&) = delete;

        MWEngine& operator=(const MWEngine&) = delete;
    };
}