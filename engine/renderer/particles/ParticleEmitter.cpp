#include "renderer/particles/ParticleEmitter.hpp"
#include "resources/ResourceManager.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>
#include <glm/gtc/constants.hpp>

namespace Engine {

ParticleEmitter::ParticleEmitter(const EmitterSettings& settings)
    : m_Settings(settings)
    , m_RNG(std::random_device{}())
{
    CreateGPUBuffers();

    // Load shaders
    auto& resources = ResourceManager::Instance();
    m_UpdateShader = resources.LoadShader("particle_update", "assets/shaders/particles/particle_update.glsl");
    m_RenderShader = resources.LoadShader("particle_render", "assets/shaders/particles/particle_render.glsl");

    // Diagnostic logging
    if (!m_UpdateShader) {
        LOG_CORE_ERROR("ParticleEmitter: Failed to load particle_update shader!");
    }
    if (!m_RenderShader) {
        LOG_CORE_ERROR("ParticleEmitter: Failed to load particle_render shader!");
    }

    // Create dummy VAO for instanced rendering (OpenGL 4.5 requires a VAO to be bound)
    glCreateVertexArrays(1, &m_DummyVAO);

    // Initialize dead list with all indices
    m_DeadList.resize(m_Settings.MaxParticles);
    for (u32 i = 0; i < m_Settings.MaxParticles; i++) {
        m_DeadList[i] = i;
    }
    m_NextFreeIndex = 0;

    // Auto-play if configured
    if (m_Settings.PlayOnStart) {
        Play();
    }
}

ParticleEmitter::~ParticleEmitter() {
    DestroyGPUBuffers();
}

ParticleEmitter::ParticleEmitter(ParticleEmitter&& other) noexcept
    : m_Settings(std::move(other.m_Settings))
    , m_State(other.m_State)
    , m_ParticleSSBO(other.m_ParticleSSBO)
    , m_CounterSSBO(other.m_CounterSSBO)
    , m_DeadListSSBO(other.m_DeadListSSBO)
    , m_UpdateShader(std::move(other.m_UpdateShader))
    , m_RenderShader(std::move(other.m_RenderShader))
    , m_CPUParticles(std::move(other.m_CPUParticles))
    , m_DeadList(std::move(other.m_DeadList))
    , m_NextFreeIndex(other.m_NextFreeIndex)
    , m_RNG(std::move(other.m_RNG))
{
    other.m_ParticleSSBO = 0;
    other.m_CounterSSBO = 0;
    other.m_DeadListSSBO = 0;
}

ParticleEmitter& ParticleEmitter::operator=(ParticleEmitter&& other) noexcept {
    if (this != &other) {
        DestroyGPUBuffers();

        m_Settings = std::move(other.m_Settings);
        m_State = other.m_State;
        m_ParticleSSBO = other.m_ParticleSSBO;
        m_CounterSSBO = other.m_CounterSSBO;
        m_DeadListSSBO = other.m_DeadListSSBO;
        m_UpdateShader = std::move(other.m_UpdateShader);
        m_RenderShader = std::move(other.m_RenderShader);
        m_CPUParticles = std::move(other.m_CPUParticles);
        m_DeadList = std::move(other.m_DeadList);
        m_NextFreeIndex = other.m_NextFreeIndex;
        m_RNG = std::move(other.m_RNG);

        other.m_ParticleSSBO = 0;
        other.m_CounterSSBO = 0;
        other.m_DeadListSSBO = 0;
    }
    return *this;
}

void ParticleEmitter::CreateGPUBuffers() {
    u32 maxParticles = m_Settings.MaxParticles;

    // Initialize CPU particle buffer
    m_CPUParticles.resize(maxParticles);
    for (auto& p : m_CPUParticles) {
        p.VelLife.w = -1.0f;  // Mark as dead
    }

    // Create particle SSBO
    glCreateBuffers(1, &m_ParticleSSBO);
    glNamedBufferStorage(m_ParticleSSBO,
        maxParticles * sizeof(GPUParticle),
        m_CPUParticles.data(),
        GL_DYNAMIC_STORAGE_BIT);

    // Create counter SSBO (aliveCount, deadCount, padding[2])
    u32 counters[4] = {0, maxParticles, 0, 0};
    glCreateBuffers(1, &m_CounterSSBO);
    glNamedBufferStorage(m_CounterSSBO, sizeof(counters), counters, GL_DYNAMIC_STORAGE_BIT);

    // Create dead list SSBO
    glCreateBuffers(1, &m_DeadListSSBO);
    glNamedBufferStorage(m_DeadListSSBO,
        maxParticles * sizeof(u32),
        nullptr,
        GL_DYNAMIC_STORAGE_BIT);

    LOG_CORE_DEBUG("ParticleEmitter: Created GPU buffers for {} particles", maxParticles);
}

void ParticleEmitter::DestroyGPUBuffers() {
    if (m_ParticleSSBO) {
        glDeleteBuffers(1, &m_ParticleSSBO);
        m_ParticleSSBO = 0;
    }
    if (m_CounterSSBO) {
        glDeleteBuffers(1, &m_CounterSSBO);
        m_CounterSSBO = 0;
    }
    if (m_DeadListSSBO) {
        glDeleteBuffers(1, &m_DeadListSSBO);
        m_DeadListSSBO = 0;
    }
    if (m_DummyVAO) {
        glDeleteVertexArrays(1, &m_DummyVAO);
        m_DummyVAO = 0;
    }
}

void ParticleEmitter::Update(f32 deltaTime) {
    if (!m_State.Playing) return;

    m_State.Time += deltaTime;

    // Handle start delay
    if (m_State.Time < m_Settings.StartDelay) {
        return;
    }

    // Check duration (if not infinite)
    if (m_Settings.Duration > 0.0f && m_State.Time >= m_Settings.Duration + m_Settings.StartDelay) {
        if (!m_Settings.Loop) {
            m_State.Playing = false;
            return;
        }
        // Reset for loop
        m_State.Time = m_Settings.StartDelay;
    }

    // Continuous spawn
    if (m_Settings.SpawnRate > 0.0f) {
        m_State.SpawnAccumulator += m_Settings.SpawnRate * deltaTime;

        while (m_State.SpawnAccumulator >= 1.0f && m_State.AliveCount < m_Settings.MaxParticles) {
            SpawnParticle();
            m_State.SpawnAccumulator -= 1.0f;
        }
    }

    // Burst spawning
    if (m_Settings.BurstCount > 0 && m_Settings.BurstInterval > 0.0f) {
        m_State.BurstTimer += deltaTime;
        if (m_State.BurstTimer >= m_Settings.BurstInterval) {
            Emit(m_Settings.BurstCount);
            m_State.BurstTimer = 0.0f;
        }
    }

    // Update particles on GPU
    UpdateGPU(deltaTime);
}

void ParticleEmitter::UpdateGPU(f32 deltaTime) {
    if (!m_UpdateShader || m_State.AliveCount == 0) return;

    m_UpdateShader->Bind();

    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ParticleSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_CounterSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_DeadListSSBO);

    // Set uniforms
    m_UpdateShader->SetFloat("u_DeltaTime", deltaTime);
    m_UpdateShader->SetFloat("u_Time", m_State.Time);
    m_UpdateShader->SetFloat3("u_Gravity", m_Settings.Gravity);
    m_UpdateShader->SetFloat("u_Drag", m_Settings.Drag);
    m_UpdateShader->SetFloat("u_Turbulence", m_Settings.Turbulence);
    m_UpdateShader->SetFloat4("u_ColorStart", m_Settings.ColorStart);
    m_UpdateShader->SetFloat4("u_ColorEnd", m_Settings.ColorEnd);
    m_UpdateShader->SetFloat("u_SizeStart", m_Settings.SizeStart);
    m_UpdateShader->SetFloat("u_SizeEnd", m_Settings.SizeEnd);

    // Dispatch compute shader
    u32 workGroups = (m_Settings.MaxParticles + 255) / 256;
    glDispatchCompute(workGroups, 1, 1);

    // Memory barrier to ensure writes are visible
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back alive count for CPU tracking
    u32 counters[4];
    glGetNamedBufferSubData(m_CounterSSBO, 0, sizeof(counters), counters);
    m_State.AliveCount = counters[0];
}

void ParticleEmitter::Render(const glm::mat4& viewProjection,
                              const glm::vec3& cameraRight,
                              const glm::vec3& cameraUp,
                              const glm::vec3& cameraPos) {
    if (!m_RenderShader || m_State.AliveCount == 0) return;

    m_RenderShader->Bind();

    // Bind particle buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ParticleSSBO);

    // Set uniforms
    m_RenderShader->SetMat4("u_ViewProjection", viewProjection);
    m_RenderShader->SetFloat3("u_CameraRight", cameraRight);
    m_RenderShader->SetFloat3("u_CameraUp", cameraUp);
    m_RenderShader->SetFloat3("u_CameraPosition", cameraPos);
    m_RenderShader->SetInt("u_BlendMode", static_cast<i32>(m_Settings.BlendMode));

    // Texture
    bool useTexture = m_Settings.Texture && m_Settings.Texture->IsLoaded();
    m_RenderShader->SetInt("u_UseTexture", useTexture ? 1 : 0);
    if (useTexture) {
        m_Settings.Texture->Bind(0);
        m_RenderShader->SetInt("u_Texture", 0);
    }

    // Set blend mode
    glEnable(GL_BLEND);
    switch (m_Settings.BlendMode) {
        case ParticleBlendMode::Additive:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case ParticleBlendMode::Alpha:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case ParticleBlendMode::Multiply:
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
        case ParticleBlendMode::Premultiplied:
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
    }

    // Disable depth write for particles (they're transparent)
    glDepthMask(GL_FALSE);

    // Bind dummy VAO - OpenGL 4.5 requires a VAO to be bound for drawing
    glBindVertexArray(m_DummyVAO);

    // Draw instanced quads (4 vertices per particle)
    // Using gl_InstanceID in shader to access particle data from SSBO
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, m_Settings.MaxParticles);

    // Restore state
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void ParticleEmitter::SpawnParticle() {
    if (m_State.AliveCount >= m_Settings.MaxParticles) return;

    // Find a dead slot
    u32 index = m_NextFreeIndex;
    m_NextFreeIndex = (m_NextFreeIndex + 1) % m_Settings.MaxParticles;

    // Check if slot is actually free (has negative lifetime)
    // In a production system, we'd use the dead list from GPU

    GPUParticle p;

    // Position
    p.PosSize.x = m_Settings.Position.x;
    p.PosSize.y = m_Settings.Position.y;
    p.PosSize.z = m_Settings.Position.z;
    glm::vec3 offset = GetSpawnPosition();
    p.PosSize.x += offset.x;
    p.PosSize.y += offset.y;
    p.PosSize.z += offset.z;

    // Size
    p.PosSize.w = m_Settings.SizeStart + RandomFloat(-m_Settings.SizeVariance, m_Settings.SizeVariance);

    // Velocity
    glm::vec3 vel = GetSpawnVelocity();
    p.VelLife.x = vel.x;
    p.VelLife.y = vel.y;
    p.VelLife.z = vel.z;

    // Lifetime
    f32 lifetime = RandomFloat(m_Settings.LifetimeMin, m_Settings.LifetimeMax);
    p.VelLife.w = lifetime;

    // Color
    p.Color = m_Settings.ColorStart;

    // Params: age, rotation, angularVelocity, maxLifetime
    p.Params.x = 0.0f;  // Age
    p.Params.y = RandomFloat(m_Settings.RotationMin, m_Settings.RotationMax);
    p.Params.z = RandomFloat(m_Settings.AngularVelocityMin, m_Settings.AngularVelocityMax);
    p.Params.w = lifetime;  // Max lifetime for interpolation

    // Upload single particle to GPU
    glNamedBufferSubData(m_ParticleSSBO, index * sizeof(GPUParticle), sizeof(GPUParticle), &p);

    m_State.AliveCount++;
    m_State.TotalEmitted++;
}

void ParticleEmitter::Emit(u32 count) {
    for (u32 i = 0; i < count && m_State.AliveCount < m_Settings.MaxParticles; i++) {
        SpawnParticle();
    }
}

void ParticleEmitter::Play() {
    m_State.Playing = true;
}

void ParticleEmitter::Pause() {
    m_State.Playing = false;
}

void ParticleEmitter::Stop() {
    m_State.Playing = false;
    Reset();
}

void ParticleEmitter::Reset() {
    m_State.Time = 0.0f;
    m_State.SpawnAccumulator = 0.0f;
    m_State.BurstTimer = 0.0f;
    m_State.AliveCount = 0;
    m_State.TotalEmitted = 0;
    m_NextFreeIndex = 0;

    // Clear all particles on GPU
    for (auto& p : m_CPUParticles) {
        p.VelLife.w = -1.0f;
    }
    glNamedBufferSubData(m_ParticleSSBO, 0, m_CPUParticles.size() * sizeof(GPUParticle), m_CPUParticles.data());

    // Reset counters
    u32 counters[4] = {0, m_Settings.MaxParticles, 0, 0};
    glNamedBufferSubData(m_CounterSSBO, 0, sizeof(counters), counters);
}

bool ParticleEmitter::IsFinished() const {
    if (m_Settings.Loop) return false;
    if (m_Settings.Duration > 0.0f) {
        return m_State.Time >= m_Settings.Duration + m_Settings.StartDelay && m_State.AliveCount == 0;
    }
    return !m_State.Playing && m_State.AliveCount == 0;
}

f32 ParticleEmitter::RandomFloat(f32 min, f32 max) {
    return min + m_Dist(m_RNG) * (max - min);
}

glm::vec3 ParticleEmitter::RandomVec3(const glm::vec3& min, const glm::vec3& max) {
    return glm::vec3(
        RandomFloat(min.x, max.x),
        RandomFloat(min.y, max.y),
        RandomFloat(min.z, max.z)
    );
}

glm::vec3 ParticleEmitter::GetSpawnPosition() {
    switch (m_Settings.Shape) {
        case EmitterShape::Point:
            return glm::vec3(0.0f);

        case EmitterShape::Box:
            return RandomVec3(-m_Settings.ShapeSize * 0.5f, m_Settings.ShapeSize * 0.5f);

        case EmitterShape::Sphere: {
            // Random point in sphere
            f32 u = RandomFloat(0.0f, 1.0f);
            f32 v = RandomFloat(0.0f, 1.0f);
            f32 theta = 2.0f * glm::pi<f32>() * u;
            f32 phi = glm::acos(2.0f * v - 1.0f);
            f32 r = m_Settings.ShapeSize.x * glm::pow(RandomFloat(0.0f, 1.0f), 1.0f / 3.0f);
            return glm::vec3(
                r * glm::sin(phi) * glm::cos(theta),
                r * glm::sin(phi) * glm::sin(theta),
                r * glm::cos(phi)
            );
        }

        case EmitterShape::Circle: {
            f32 angle = RandomFloat(0.0f, glm::two_pi<f32>());
            f32 r = m_Settings.ShapeSize.x * glm::sqrt(RandomFloat(0.0f, 1.0f));
            return glm::vec3(r * glm::cos(angle), 0.0f, r * glm::sin(angle));
        }

        case EmitterShape::Line: {
            f32 t = RandomFloat(0.0f, 1.0f);
            return glm::vec3(t * m_Settings.ShapeSize.x - m_Settings.ShapeSize.x * 0.5f, 0.0f, 0.0f);
        }

        default:
            return glm::vec3(0.0f);
    }
}

glm::vec3 ParticleEmitter::GetSpawnVelocity() {
    glm::vec3 vel = RandomVec3(m_Settings.VelocityMin, m_Settings.VelocityMax);

    // Apply speed range
    f32 speed = RandomFloat(m_Settings.SpeedMin, m_Settings.SpeedMax);
    if (glm::length(vel) > 0.001f) {
        vel = glm::normalize(vel) * speed;
    } else {
        // Random direction if velocity is zero
        vel = glm::vec3(
            RandomFloat(-1.0f, 1.0f),
            RandomFloat(-1.0f, 1.0f),
            RandomFloat(-1.0f, 1.0f)
        );
        if (glm::length(vel) > 0.001f) {
            vel = glm::normalize(vel) * speed;
        }
    }

    return vel;
}

} // namespace Engine
