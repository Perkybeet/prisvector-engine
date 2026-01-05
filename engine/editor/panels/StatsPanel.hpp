#pragma once

#include "Panel.hpp"
#include "core/Types.hpp"

namespace Editor {

class StatsPanel : public Panel {
public:
    StatsPanel() : Panel("Statistics") {}

    void OnImGuiRender() override;

private:
    Engine::Vector<Engine::f32> m_FrameTimes;
    Engine::f32 m_SmoothedFPS = 60.0f;
    Engine::f32 m_SmoothedFrameTime = 16.67f;
    Engine::f32 m_LastFPSUpdate = 0.0f;
    static constexpr Engine::u32 SampleCount = 60;
    static constexpr Engine::f32 UpdateInterval = 0.5f;
};

} // namespace Editor
