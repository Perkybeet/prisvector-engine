#type compute
#version 450 core

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// GPU Particle struct
struct Particle {
    vec4 posSize;      // xyz = position, w = size
    vec4 velLife;      // xyz = velocity, w = remaining lifetime
    vec4 color;        // rgba
    vec4 params;       // x = age, y = rotation, z = angularVel, w = maxLife
};

// Particle buffer (read/write)
layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

// Atomic counters
layout(std430, binding = 1) buffer CounterBuffer {
    uint aliveCount;
    uint deadCount;
    uint padding[2];
};

// Dead particle indices for recycling
layout(std430, binding = 2) buffer DeadList {
    uint deadIndices[];
};

// Uniforms
uniform float u_DeltaTime;
uniform float u_Time;
uniform vec3 u_Gravity;
uniform float u_Drag;
uniform float u_Turbulence;

// Color interpolation
uniform vec4 u_ColorStart;
uniform vec4 u_ColorEnd;

// Size interpolation
uniform float u_SizeStart;
uniform float u_SizeEnd;

// Simple pseudo-random function
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 randomDirection(uint seed) {
    float u = rand(vec2(float(seed), u_Time)) * 2.0 - 1.0;
    float theta = rand(vec2(u_Time, float(seed))) * 6.28318;
    float s = sqrt(1.0 - u * u);
    return vec3(s * cos(theta), s * sin(theta), u);
}

void main() {
    uint idx = gl_GlobalInvocationID.x;

    // Bounds check
    if (idx >= particles.length()) {
        return;
    }

    Particle p = particles[idx];

    // Skip already dead particles
    if (p.velLife.w <= 0.0) {
        return;
    }

    // Update lifetime
    float dt = u_DeltaTime;
    p.velLife.w -= dt;
    p.params.x += dt;  // Increment age

    // Check if particle just died
    if (p.velLife.w <= 0.0) {
        // Add to dead list for recycling
        uint deadIdx = atomicAdd(deadCount, 1);
        if (deadIdx < deadIndices.length()) {
            deadIndices[deadIdx] = idx;
        }
        atomicSub(aliveCount, 1);

        // Mark as dead
        p.color.a = 0.0;
        p.velLife.w = -1.0;  // Ensure it's negative

        particles[idx] = p;
        return;
    }

    // === Physics Update ===

    // Apply gravity
    vec3 velocity = p.velLife.xyz;
    velocity += u_Gravity * dt;

    // Apply drag (air resistance)
    velocity *= (1.0 - u_Drag * dt);

    // Apply turbulence (random force)
    if (u_Turbulence > 0.0) {
        vec3 turbForce = randomDirection(idx) * u_Turbulence;
        velocity += turbForce * dt;
    }

    // Update position
    p.posSize.xyz += velocity * dt;
    p.velLife.xyz = velocity;

    // === Visual Update ===

    // Calculate normalized lifetime (0 = just born, 1 = about to die)
    float lifeRatio = p.params.x / p.params.w;
    lifeRatio = clamp(lifeRatio, 0.0, 1.0);

    // Interpolate color
    p.color = mix(u_ColorStart, u_ColorEnd, lifeRatio);

    // Interpolate size
    p.posSize.w = mix(u_SizeStart, u_SizeEnd, lifeRatio);

    // Update rotation
    p.params.y += p.params.z * dt;  // rotation += angularVelocity * dt

    // Write back
    particles[idx] = p;
}
