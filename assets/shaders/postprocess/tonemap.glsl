#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;

out vec2 v_TexCoords;

void main() {
    v_TexCoords = a_TexCoords;
    gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

out vec4 FragColor;
in vec2 v_TexCoords;

uniform sampler2D u_HDRBuffer;
uniform float u_Exposure;
uniform float u_Gamma;

// ACES Filmic Tone Mapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Reinhard Tone Mapping
vec3 Reinhard(vec3 x) {
    return x / (x + vec3(1.0));
}

// Uncharted 2 Tone Mapping
vec3 Uncharted2Tonemap(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

void main() {
    vec3 hdrColor = texture(u_HDRBuffer, v_TexCoords).rgb;

    // Exposure adjustment
    vec3 mapped = hdrColor * u_Exposure;

    // Tone mapping (ACES looks best for PBR)
    mapped = ACESFilm(mapped);

    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / u_Gamma));

    FragColor = vec4(mapped, 1.0);
}
