#type vertex
#version 450 core

// GPU Particle struct (must match C++ GPUParticle layout)
struct Particle {
    vec4 posSize;      // xyz = position, w = size
    vec4 velLife;      // xyz = velocity, w = remaining lifetime
    vec4 color;        // rgba
    vec4 params;       // x = age, y = rotation, z = angularVel, w = maxLife
};

layout(std430, binding = 0) readonly buffer ParticleBuffer {
    Particle particles[];
};

// Camera uniforms
uniform mat4 u_ViewProjection;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;
uniform vec3 u_CameraPosition;

// Outputs
out vec4 v_Color;
out vec2 v_TexCoord;

// Quad corners (4 vertices per particle, using gl_VertexID)
const vec2 QUAD_CORNERS[4] = vec2[](
    vec2(-0.5, -0.5),  // Bottom-left
    vec2( 0.5, -0.5),  // Bottom-right
    vec2( 0.5,  0.5),  // Top-right
    vec2(-0.5,  0.5)   // Top-left
);

const vec2 QUAD_UVS[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

void main() {
    // Get particle from buffer using instance ID
    uint particleIndex = gl_InstanceID;
    Particle p = particles[particleIndex];

    // Skip dead particles (lifetime <= 0)
    if (p.velLife.w <= 0.0) {
        gl_Position = vec4(-1000.0, -1000.0, -1000.0, 1.0);  // Off-screen
        v_Color = vec4(0.0);
        v_TexCoord = vec2(0.0);
        return;
    }

    // Get corner for this vertex
    int vertexIndex = gl_VertexID % 4;
    vec2 corner = QUAD_CORNERS[vertexIndex];

    // Apply rotation
    float rotation = p.params.y;
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    vec2 rotatedCorner = vec2(
        corner.x * cosR - corner.y * sinR,
        corner.x * sinR + corner.y * cosR
    );

    // Scale by particle size
    float size = p.posSize.w;
    rotatedCorner *= size;

    // Billboard: offset in camera space
    vec3 worldPos = p.posSize.xyz;
    worldPos += u_CameraRight * rotatedCorner.x;
    worldPos += u_CameraUp * rotatedCorner.y;

    // Transform to clip space
    gl_Position = u_ViewProjection * vec4(worldPos, 1.0);

    // Pass color and UVs
    v_Color = p.color;
    v_TexCoord = QUAD_UVS[vertexIndex];
}

#type fragment
#version 450 core

in vec4 v_Color;
in vec2 v_TexCoord;

out vec4 FragColor;

uniform sampler2D u_Texture;
uniform bool u_UseTexture;
uniform int u_BlendMode;  // 0=Additive, 1=Alpha, 2=Multiply

void main() {
    vec4 color = v_Color;

    // Sample texture if available
    if (u_UseTexture) {
        vec4 texColor = texture(u_Texture, v_TexCoord);
        color *= texColor;
    } else {
        // Soft circular falloff for particles without texture
        float dist = length(v_TexCoord - 0.5) * 2.0;
        float alpha = 1.0 - smoothstep(0.0, 1.0, dist);
        color.a *= alpha;
    }

    // Discard fully transparent pixels
    if (color.a < 0.01) {
        discard;
    }

    FragColor = color;
}
