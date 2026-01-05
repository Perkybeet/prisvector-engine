#pragma once

#include "ParticleEmitter.hpp"
#include "camera/Camera.hpp"

namespace Engine {

// Standalone particle system for use in demos (non-ECS)
class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    // Lifecycle
    void Initialize();
    void Shutdown();
    void Update(f32 deltaTime);
    void Render();

    // Camera (required for billboarding)
    void SetCamera(Camera* camera) { m_Camera = camera; }

    // Emitter management
    ParticleEmitter* CreateEmitter(const EmitterSettings& settings);
    void DestroyEmitter(ParticleEmitter* emitter);
    void ClearAllEmitters();

    // Global controls
    void SetTimeScale(f32 scale) { m_TimeScale = scale; }
    f32 GetTimeScale() const { return m_TimeScale; }

    void PauseAll();
    void ResumeAll();

    // Stats
    const ParticleStats& GetStats() const { return m_Stats; }

private:
    void UpdateStats();

private:
    Vector<Scope<ParticleEmitter>> m_Emitters;
    Camera* m_Camera = nullptr;
    f32 m_TimeScale = 1.0f;
    bool m_Initialized = false;
    ParticleStats m_Stats;
};

} // namespace Engine
