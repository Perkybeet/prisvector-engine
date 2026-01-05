#pragma once

#include "ParticleTypes.hpp"
#include "renderer/opengl/GLShader.hpp"
#include <random>

namespace Engine {

class ParticleEmitter {
public:
    explicit ParticleEmitter(const EmitterSettings& settings);
    ~ParticleEmitter();

    // Non-copyable
    ParticleEmitter(const ParticleEmitter&) = delete;
    ParticleEmitter& operator=(const ParticleEmitter&) = delete;

    // Movable
    ParticleEmitter(ParticleEmitter&& other) noexcept;
    ParticleEmitter& operator=(ParticleEmitter&& other) noexcept;

    // Lifecycle
    void Update(f32 deltaTime);
    void Render(const glm::mat4& viewProjection, const glm::vec3& cameraRight,
                const glm::vec3& cameraUp, const glm::vec3& cameraPos);

    // Control
    void Play();
    void Pause();
    void Stop();
    void Reset();
    void Emit(u32 count);  // Burst emit

    // State
    bool IsPlaying() const { return m_State.Playing; }
    bool IsFinished() const;
    u32 GetAliveCount() const { return m_State.AliveCount; }

    // Transform
    void SetPosition(const glm::vec3& position) { m_Settings.Position = position; }
    glm::vec3 GetPosition() const { return m_Settings.Position; }

    // Settings access
    EmitterSettings& GetSettings() { return m_Settings; }
    const EmitterSettings& GetSettings() const { return m_Settings; }

    // GPU buffer access (for external rendering)
    u32 GetParticleSSBO() const { return m_ParticleSSBO; }
    u32 GetMaxParticles() const { return m_Settings.MaxParticles; }

private:
    void CreateGPUBuffers();
    void DestroyGPUBuffers();
    void SpawnParticle();
    void UpdateGPU(f32 deltaTime);

    // Random helpers
    f32 RandomFloat(f32 min, f32 max);
    glm::vec3 RandomVec3(const glm::vec3& min, const glm::vec3& max);
    glm::vec3 GetSpawnPosition();
    glm::vec3 GetSpawnVelocity();

private:
    EmitterSettings m_Settings;
    EmitterState m_State;

    // GPU resources
    u32 m_ParticleSSBO = 0;     // Particle data buffer
    u32 m_CounterSSBO = 0;      // Alive/dead counters
    u32 m_DeadListSSBO = 0;     // Dead particle indices
    u32 m_DummyVAO = 0;         // Dummy VAO for instanced rendering

    // Shaders
    Ref<Shader> m_UpdateShader;
    Ref<Shader> m_RenderShader;

    // CPU particle buffer for spawning
    Vector<GPUParticle> m_CPUParticles;
    Vector<u32> m_DeadList;
    u32 m_NextFreeIndex = 0;

    // Random number generator
    std::mt19937 m_RNG;
    std::uniform_real_distribution<f32> m_Dist{0.0f, 1.0f};
};

} // namespace Engine
