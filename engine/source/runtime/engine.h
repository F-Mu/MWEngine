#pragma once


namespace MW
{
    class MWEngine
    {
        friend class MWEditor;
        static const float fpsAlpha;
    public:
        MWEngine() {};
        ~MWEngine() {};
        void run();

        void tick(float deltaTime);

        void clear();
        void initialize();
        void logicalTick(float deltaTime);
        void calculateFPS(float deltaTime);
        MWEngine(const MWEngine&) = delete;

        MWEngine& operator=(const MWEngine&) = delete;
    protected:
        float averageDuration {0.f};
        int   frameCount {0};
        int   fps {0};
    };
}