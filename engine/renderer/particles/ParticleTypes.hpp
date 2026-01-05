#pragma once

#include "core/Types.hpp"
#include "renderer/Texture.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace Engine {

// GPU-aligned particle struct (64 bytes, matches GLSL layout)
struct GPUParticle {
    glm::vec4 PosSize;      // xyz = position, w = size
    glm::vec4 VelLife;      // xyz = velocity, w = remaining lifetime
    glm::vec4 Color;        // rgba
    glm::vec4 Params;       // x = age, y = rotation, z = angularVel, w = maxLife
};

static_assert(sizeof(GPUParticle) == 64, "GPUParticle must be 64 bytes for GPU alignment");

// Blend modes for particle rendering
enum class ParticleBlendMode : u32 {
    Additive = 0,   // Fire, sparks, glow effects
    Alpha = 1,      // Smoke, dust, clouds
    Multiply = 2,   // Dark effects, shadows
    Premultiplied = 3  // Pre-multiplied alpha
};

// Emitter shape for spawn positions
enum class EmitterShape : u32 {
    Point = 0,      // All particles spawn at center
    Sphere = 1,     // Random positions on/in sphere
    Box = 2,        // Random positions in box volume
    Cone = 3,       // Conical emission direction
    Circle = 4,     // 2D circle (horizontal)
    Line = 5        // Along a line segment
};

// Particle emitter settings
struct EmitterSettings {
    // === Spawn Settings ===
    u32 MaxParticles = 10000;
    f32 SpawnRate = 100.0f;         // Particles per second
    u32 BurstCount = 0;             // Instant burst on emit
    f32 BurstInterval = 0.0f;       // Time between auto-bursts (0 = no auto)

    // === Shape & Position ===
    EmitterShape Shape = EmitterShape::Point;
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 ShapeSize = glm::vec3(1.0f);   // Box size or sphere radius
    f32 ConeAngle = 45.0f;                   // For cone shape

    // === Initial Velocity ===
    glm::vec3 VelocityMin = glm::vec3(-0.5f, 1.0f, -0.5f);
    glm::vec3 VelocityMax = glm::vec3(0.5f, 3.0f, 0.5f);
    f32 SpeedMin = 1.0f;
    f32 SpeedMax = 3.0f;

    // === Lifetime ===
    f32 LifetimeMin = 1.0f;
    f32 LifetimeMax = 3.0f;

    // === Size ===
    f32 SizeStart = 0.1f;
    f32 SizeEnd = 0.5f;
    f32 SizeVariance = 0.1f;        // Random variation

    // === Color ===
    glm::vec4 ColorStart = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 ColorEnd = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    glm::vec4 ColorVariance = glm::vec4(0.0f);  // HSV variance

    // === Rotation ===
    f32 RotationMin = 0.0f;
    f32 RotationMax = 0.0f;
    f32 AngularVelocityMin = 0.0f;
    f32 AngularVelocityMax = 0.0f;

    // === Physics ===
    glm::vec3 Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    f32 Drag = 0.1f;
    f32 Turbulence = 0.0f;          // Random force strength

    // === Rendering ===
    ParticleBlendMode BlendMode = ParticleBlendMode::Additive;
    Ref<Texture2D> Texture = nullptr;
    u32 SpriteSheetRows = 1;
    u32 SpriteSheetCols = 1;
    bool AnimateSprite = false;
    f32 SpriteAnimSpeed = 1.0f;

    // === Emission ===
    bool Loop = true;
    bool PlayOnStart = true;
    f32 Duration = 0.0f;            // 0 = infinite
    f32 StartDelay = 0.0f;
};

// Particle emitter runtime state
struct EmitterState {
    bool Playing = false;
    f32 Time = 0.0f;
    f32 SpawnAccumulator = 0.0f;
    f32 BurstTimer = 0.0f;
    u32 AliveCount = 0;
    u32 TotalEmitted = 0;
};

// Particle system stats
struct ParticleStats {
    u32 TotalEmitters = 0;
    u32 ActiveEmitters = 0;
    u32 TotalParticles = 0;
    u32 AliveParticles = 0;
    f32 UpdateTimeMs = 0.0f;
    f32 RenderTimeMs = 0.0f;
};

// Preset configurations
namespace ParticlePresets {

inline EmitterSettings Fire() {
    EmitterSettings s;
    s.MaxParticles = 500;
    s.SpawnRate = 50.0f;
    s.Shape = EmitterShape::Circle;
    s.ShapeSize = glm::vec3(0.3f);

    s.VelocityMin = glm::vec3(-0.2f, 2.0f, -0.2f);
    s.VelocityMax = glm::vec3(0.2f, 4.0f, 0.2f);

    s.LifetimeMin = 0.5f;
    s.LifetimeMax = 1.5f;

    s.SizeStart = 0.3f;
    s.SizeEnd = 0.8f;
    s.SizeVariance = 0.1f;

    s.ColorStart = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);   // Yellow-orange
    s.ColorEnd = glm::vec4(1.0f, 0.2f, 0.0f, 0.0f);      // Red, fade out

    s.Gravity = glm::vec3(0.0f, 1.0f, 0.0f);             // Upward (fire rises)
    s.Drag = 0.3f;
    s.Turbulence = 0.5f;

    s.BlendMode = ParticleBlendMode::Additive;
    return s;
}

inline EmitterSettings Smoke() {
    EmitterSettings s;
    s.MaxParticles = 300;
    s.SpawnRate = 20.0f;
    s.Shape = EmitterShape::Circle;
    s.ShapeSize = glm::vec3(0.5f);

    s.VelocityMin = glm::vec3(-0.3f, 0.5f, -0.3f);
    s.VelocityMax = glm::vec3(0.3f, 1.5f, 0.3f);

    s.LifetimeMin = 2.0f;
    s.LifetimeMax = 4.0f;

    s.SizeStart = 0.2f;
    s.SizeEnd = 1.5f;

    s.ColorStart = glm::vec4(0.3f, 0.3f, 0.3f, 0.6f);
    s.ColorEnd = glm::vec4(0.1f, 0.1f, 0.1f, 0.0f);

    s.RotationMin = 0.0f;
    s.RotationMax = glm::two_pi<f32>();
    s.AngularVelocityMin = -0.5f;
    s.AngularVelocityMax = 0.5f;

    s.Gravity = glm::vec3(0.0f, 0.5f, 0.0f);
    s.Drag = 0.5f;
    s.Turbulence = 0.8f;

    s.BlendMode = ParticleBlendMode::Alpha;
    return s;
}

inline EmitterSettings Sparks() {
    EmitterSettings s;
    s.MaxParticles = 200;
    s.SpawnRate = 30.0f;
    s.BurstCount = 20;
    s.BurstInterval = 0.5f;
    s.Shape = EmitterShape::Point;

    s.VelocityMin = glm::vec3(-2.0f, 1.0f, -2.0f);
    s.VelocityMax = glm::vec3(2.0f, 5.0f, 2.0f);

    s.LifetimeMin = 0.3f;
    s.LifetimeMax = 0.8f;

    s.SizeStart = 0.05f;
    s.SizeEnd = 0.02f;

    s.ColorStart = glm::vec4(1.0f, 0.9f, 0.5f, 1.0f);    // Bright yellow
    s.ColorEnd = glm::vec4(1.0f, 0.3f, 0.0f, 0.0f);      // Orange, fade

    s.Gravity = glm::vec3(0.0f, -9.81f, 0.0f);           // Real gravity
    s.Drag = 0.05f;

    s.BlendMode = ParticleBlendMode::Additive;
    return s;
}

inline EmitterSettings Explosion() {
    EmitterSettings s;
    s.MaxParticles = 1000;
    s.SpawnRate = 0.0f;             // No continuous spawn
    s.BurstCount = 500;             // Single big burst
    s.Shape = EmitterShape::Sphere;
    s.ShapeSize = glm::vec3(0.2f);

    s.SpeedMin = 5.0f;
    s.SpeedMax = 15.0f;

    s.LifetimeMin = 0.5f;
    s.LifetimeMax = 1.5f;

    s.SizeStart = 0.3f;
    s.SizeEnd = 0.1f;

    s.ColorStart = glm::vec4(1.0f, 0.7f, 0.3f, 1.0f);
    s.ColorEnd = glm::vec4(0.5f, 0.1f, 0.0f, 0.0f);

    s.Gravity = glm::vec3(0.0f, -3.0f, 0.0f);
    s.Drag = 0.8f;

    s.BlendMode = ParticleBlendMode::Additive;
    s.Loop = false;
    return s;
}

inline EmitterSettings Rain() {
    EmitterSettings s;
    s.MaxParticles = 5000;
    s.SpawnRate = 500.0f;
    s.Shape = EmitterShape::Box;
    s.ShapeSize = glm::vec3(20.0f, 0.1f, 20.0f);
    s.Position = glm::vec3(0.0f, 15.0f, 0.0f);

    s.VelocityMin = glm::vec3(-0.5f, -10.0f, -0.5f);
    s.VelocityMax = glm::vec3(0.5f, -8.0f, 0.5f);

    s.LifetimeMin = 1.5f;
    s.LifetimeMax = 2.0f;

    s.SizeStart = 0.02f;
    s.SizeEnd = 0.02f;

    s.ColorStart = glm::vec4(0.7f, 0.8f, 1.0f, 0.5f);
    s.ColorEnd = glm::vec4(0.7f, 0.8f, 1.0f, 0.3f);

    s.Gravity = glm::vec3(0.0f, -2.0f, 0.0f);            // Extra gravity
    s.Drag = 0.0f;

    s.BlendMode = ParticleBlendMode::Alpha;
    return s;
}

inline EmitterSettings Snow() {
    EmitterSettings s;
    s.MaxParticles = 2000;
    s.SpawnRate = 100.0f;
    s.Shape = EmitterShape::Box;
    s.ShapeSize = glm::vec3(20.0f, 0.1f, 20.0f);
    s.Position = glm::vec3(0.0f, 15.0f, 0.0f);

    s.VelocityMin = glm::vec3(-0.5f, -1.0f, -0.5f);
    s.VelocityMax = glm::vec3(0.5f, -0.5f, 0.5f);

    s.LifetimeMin = 8.0f;
    s.LifetimeMax = 12.0f;

    s.SizeStart = 0.05f;
    s.SizeEnd = 0.05f;
    s.SizeVariance = 0.03f;

    s.ColorStart = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f);
    s.ColorEnd = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);

    s.RotationMin = 0.0f;
    s.RotationMax = glm::two_pi<f32>();
    s.AngularVelocityMin = -1.0f;
    s.AngularVelocityMax = 1.0f;

    s.Gravity = glm::vec3(0.0f, -0.3f, 0.0f);
    s.Drag = 0.1f;
    s.Turbulence = 1.0f;

    s.BlendMode = ParticleBlendMode::Alpha;
    return s;
}

inline EmitterSettings Magic() {
    EmitterSettings s;
    s.MaxParticles = 300;
    s.SpawnRate = 40.0f;
    s.Shape = EmitterShape::Sphere;
    s.ShapeSize = glm::vec3(0.5f);

    s.VelocityMin = glm::vec3(-0.5f, -0.5f, -0.5f);
    s.VelocityMax = glm::vec3(0.5f, 0.5f, 0.5f);

    s.LifetimeMin = 1.0f;
    s.LifetimeMax = 2.0f;

    s.SizeStart = 0.15f;
    s.SizeEnd = 0.05f;

    s.ColorStart = glm::vec4(0.5f, 0.2f, 1.0f, 1.0f);    // Purple
    s.ColorEnd = glm::vec4(0.2f, 0.8f, 1.0f, 0.0f);      // Cyan fade

    s.Gravity = glm::vec3(0.0f);
    s.Drag = 0.2f;
    s.Turbulence = 1.5f;

    s.BlendMode = ParticleBlendMode::Additive;
    return s;
}

} // namespace ParticlePresets

} // namespace Engine
