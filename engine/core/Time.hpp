#pragma once

#include "Types.hpp"

namespace Engine {

class Time {
public:
    static void Init();
    static void Update();

    static f32 GetTime();
    static f32 GetDeltaTime() { return s_DeltaTime; }
    static f32 GetUnscaledDeltaTime() { return s_DeltaTime; }
    static f32 GetScaledDeltaTime() { return s_DeltaTime * s_TimeScale; }
    static f32 GetFPS() { return 1.0f / s_DeltaTime; }

    static void SetTimeScale(f32 scale) { s_TimeScale = scale; }
    static f32 GetTimeScale() { return s_TimeScale; }
    static bool IsPaused() { return s_TimeScale == 0.0f; }

private:
    static f32 s_DeltaTime;
    static f32 s_LastFrameTime;
    static f32 s_TimeScale;
};

} // namespace Engine
