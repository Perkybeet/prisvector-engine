#include "renderer/particles/ParticleSystem.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>
#include <algorithm>

namespace Engine {

ParticleSystem::ParticleSystem() {
    m_Emitters.reserve(32);
}

ParticleSystem::~ParticleSystem() {
    Shutdown();
}

void ParticleSystem::Initialize() {
    if (m_Initialized) return;

    LOG_CORE_INFO("ParticleSystem initialized");
    m_Initialized = true;
}

void ParticleSystem::Shutdown() {
    if (!m_Initialized) return;

    ClearAllEmitters();
    m_Initialized = false;

    LOG_CORE_INFO("ParticleSystem shutdown");
}

void ParticleSystem::Update(f32 deltaTime) {
    if (!m_Initialized) return;

    f32 scaledDt = deltaTime * m_TimeScale;

    // Update all emitters
    for (auto& emitter : m_Emitters) {
        if (emitter) {
            emitter->Update(scaledDt);
        }
    }

    // Remove finished non-looping emitters
    m_Emitters.erase(
        std::remove_if(m_Emitters.begin(), m_Emitters.end(),
            [](const Scope<ParticleEmitter>& e) {
                return e && e->IsFinished() && !e->GetSettings().Loop;
            }),
        m_Emitters.end()
    );

    UpdateStats();
}

void ParticleSystem::Render() {
    if (!m_Initialized) {
        LOG_CORE_WARN("ParticleSystem::Render() called but system not initialized");
        return;
    }
    if (!m_Camera) {
        LOG_CORE_WARN("ParticleSystem::Render() called but no camera set (call SetCamera first)");
        return;
    }

    // Get camera vectors for billboarding
    glm::mat4 viewProj = m_Camera->GetViewProjectionMatrix();
    glm::vec3 cameraPos = m_Camera->GetPosition();

    // Extract right and up vectors from view matrix
    glm::mat4 view = m_Camera->GetViewMatrix();
    glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);

    // Sort emitters by blend mode (draw alpha-blended last)
    // For now, just render in order

    // Disable depth writing for transparent particles
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);

    for (auto& emitter : m_Emitters) {
        if (emitter && emitter->GetAliveCount() > 0) {
            emitter->Render(viewProj, cameraRight, cameraUp, cameraPos);
        }
    }

    // Restore state
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

ParticleEmitter* ParticleSystem::CreateEmitter(const EmitterSettings& settings) {
    auto emitter = CreateScope<ParticleEmitter>(settings);
    ParticleEmitter* ptr = emitter.get();
    m_Emitters.push_back(std::move(emitter));

    LOG_CORE_DEBUG("ParticleSystem: Created emitter (max {} particles)", settings.MaxParticles);
    return ptr;
}

void ParticleSystem::DestroyEmitter(ParticleEmitter* emitter) {
    m_Emitters.erase(
        std::remove_if(m_Emitters.begin(), m_Emitters.end(),
            [emitter](const Scope<ParticleEmitter>& e) {
                return e.get() == emitter;
            }),
        m_Emitters.end()
    );
}

void ParticleSystem::ClearAllEmitters() {
    m_Emitters.clear();
}

void ParticleSystem::PauseAll() {
    for (auto& emitter : m_Emitters) {
        if (emitter) {
            emitter->Pause();
        }
    }
}

void ParticleSystem::ResumeAll() {
    for (auto& emitter : m_Emitters) {
        if (emitter) {
            emitter->Play();
        }
    }
}

void ParticleSystem::UpdateStats() {
    m_Stats.TotalEmitters = static_cast<u32>(m_Emitters.size());
    m_Stats.ActiveEmitters = 0;
    m_Stats.TotalParticles = 0;
    m_Stats.AliveParticles = 0;

    for (const auto& emitter : m_Emitters) {
        if (emitter) {
            m_Stats.TotalParticles += emitter->GetMaxParticles();
            m_Stats.AliveParticles += emitter->GetAliveCount();
            if (emitter->IsPlaying()) {
                m_Stats.ActiveEmitters++;
            }
        }
    }
}

} // namespace Engine
