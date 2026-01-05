#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoords;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat3 u_NormalMatrix;

out VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
} vs_out;

void main() {
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    vs_out.WorldPos = worldPos.xyz;
    vs_out.TexCoords = a_TexCoords;

    vec3 N = normalize(u_NormalMatrix * a_Normal);
    vec3 T = normalize(u_NormalMatrix * a_Tangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    vs_out.Normal = N;
    vs_out.TBN = mat3(T, B, N);

    gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec4 gEmission;

in VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

// Material uniforms
uniform vec4 u_AlbedoColor;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;
uniform vec3 u_EmissiveColor;
uniform float u_EmissiveIntensity;
uniform float u_NormalStrength;
uniform vec2 u_TilingFactor;

// Texture flags
uniform uint u_TextureFlags;

// Textures
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicRoughnessMap;
uniform sampler2D u_AOMap;
uniform sampler2D u_EmissiveMap;

// Camera
uniform float u_NearPlane;
uniform float u_FarPlane;

const uint HAS_ALBEDO = 1u;
const uint HAS_NORMAL = 2u;
const uint HAS_METALLIC_ROUGHNESS = 4u;
const uint HAS_AO = 8u;
const uint HAS_EMISSIVE = 16u;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * u_NearPlane * u_FarPlane) / (u_FarPlane + u_NearPlane - z * (u_FarPlane - u_NearPlane));
}

void main() {
    vec2 uv = fs_in.TexCoords * u_TilingFactor;

    // Albedo
    vec4 albedo = u_AlbedoColor;
    if ((u_TextureFlags & HAS_ALBEDO) != 0u) {
        albedo *= texture(u_AlbedoMap, uv);
    }

    // Normal
    vec3 normal = fs_in.Normal;
    if ((u_TextureFlags & HAS_NORMAL) != 0u) {
        vec3 tangentNormal = texture(u_NormalMap, uv).rgb * 2.0 - 1.0;
        tangentNormal.xy *= u_NormalStrength;
        normal = normalize(fs_in.TBN * tangentNormal);
    }

    // Metallic & Roughness
    float metallic = u_Metallic;
    float roughness = u_Roughness;
    if ((u_TextureFlags & HAS_METALLIC_ROUGHNESS) != 0u) {
        vec2 mr = texture(u_MetallicRoughnessMap, uv).bg;
        metallic = mr.x;
        roughness = mr.y;
    }

    // AO
    float ao = u_AO;
    if ((u_TextureFlags & HAS_AO) != 0u) {
        ao = texture(u_AOMap, uv).r;
    }

    // Emission
    vec3 emission = u_EmissiveColor * u_EmissiveIntensity;
    if ((u_TextureFlags & HAS_EMISSIVE) != 0u) {
        emission = texture(u_EmissiveMap, uv).rgb * u_EmissiveIntensity;
    }

    // Output to G-Buffer
    float linearDepth = LinearizeDepth(gl_FragCoord.z);

    gPosition = vec4(fs_in.WorldPos, linearDepth);
    gNormal = vec4(normal * 0.5 + 0.5, metallic);
    gAlbedo = vec4(albedo.rgb, roughness);
    gEmission = vec4(emission, ao);
}
