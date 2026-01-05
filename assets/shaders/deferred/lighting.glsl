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

// G-Buffer textures
uniform sampler2D u_GPosition;
uniform sampler2D u_GNormal;
uniform sampler2D u_GAlbedo;
uniform sampler2D u_GEmission;

// Shadow maps
uniform sampler2DArray u_CSMShadowMap;
uniform sampler2D u_SpotShadowAtlas;

// Camera
uniform vec3 u_CameraPos;

// Ambient
uniform vec4 u_AmbientLight;

// Light counts
uniform int u_DirectionalLightCount;
uniform int u_PointLightCount;
uniform int u_SpotLightCount;

// Shadow settings
uniform bool u_ShadowsEnabled;

// ============================================================================
// Light UBO definitions
// ============================================================================

struct DirectionalLight {
    vec4 direction;
    vec4 colorIntensity;
};

struct PointLight {
    vec4 position;
    vec4 colorIntensity;
    vec4 attenuation;
};

struct SpotLight {
    vec4 position;
    vec4 direction;
    vec4 colorIntensity;
    vec4 cutoffAtten;
};

layout(std140, binding = 0) uniform DirectionalLightBlock {
    DirectionalLight u_DirectionalLights[4];
};

layout(std140, binding = 1) uniform PointLightBlock {
    PointLight u_PointLights[256];
};

layout(std140, binding = 2) uniform SpotLightBlock {
    SpotLight u_SpotLights[64];
};

// ============================================================================
// Shadow UBO definitions
// ============================================================================

struct CascadeData {
    mat4 viewProjection;
    vec4 splitDepthBias;  // x=splitDepth, y=texelSize, z=bias, w=normalBias
};

struct SpotShadowData {
    mat4 viewProjection;
    vec4 atlasScaleOffset;  // xy=scale, zw=offset
    vec4 params;            // x=bias, y=normalBias, z=softness, w=enabled
};

layout(std140, binding = 3) uniform ShadowDataBlock {
    // CSM data
    CascadeData u_Cascades[4];
    vec4 u_CascadeSplitDepths;
    vec4 u_ShadowParams;  // x=softness, y=maxDist, z=fadeStart, w=enabled

    // Spot shadow data
    SpotShadowData u_SpotShadows[16];
    ivec4 u_ShadowCounts;  // x=spotCount
};

// ============================================================================
// PBR Functions (Cook-Torrance BRDF)
// ============================================================================

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ============================================================================
// Shadow Functions
// ============================================================================

const int PCF_SAMPLES = 16;
const vec2 POISSON_DISK[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
);

int GetCascadeIndex(float viewDepth) {
    int cascadeIndex = 0;
    for (int i = 0; i < 4; i++) {
        if (viewDepth > u_CascadeSplitDepths[i]) {
            cascadeIndex = i + 1;
        }
    }
    return min(cascadeIndex, 3);
}

float SampleShadowPCF(vec3 shadowCoord, int cascadeIndex, float softness, float texelSize, float bias) {
    float shadow = 0.0;
    float currentDepth = shadowCoord.z;

    for (int i = 0; i < PCF_SAMPLES; i++) {
        vec2 offset = POISSON_DISK[i] * softness * texelSize;
        float closestDepth = texture(u_CSMShadowMap, vec3(shadowCoord.xy + offset, float(cascadeIndex))).r;
        shadow += (currentDepth - bias) > closestDepth ? 0.0 : 1.0;
    }

    return shadow / float(PCF_SAMPLES);
}

float CalculateCSMShadow(vec3 worldPos, vec3 normal, float viewDepth) {
    // Check if shadows are disabled or beyond max distance
    if (u_ShadowParams.w < 0.5 || viewDepth > u_ShadowParams.y) {
        return 1.0;
    }

    // Select cascade
    int cascadeIndex = GetCascadeIndex(viewDepth);

    // Get cascade parameters
    float bias = u_Cascades[cascadeIndex].splitDepthBias.z;
    float normalBias = u_Cascades[cascadeIndex].splitDepthBias.w;
    float texelSize = u_Cascades[cascadeIndex].splitDepthBias.y;
    float softness = u_ShadowParams.x;

    // Apply normal offset bias
    vec3 samplingPos = worldPos + normal * normalBias;

    // Transform to light space
    vec4 shadowPos = u_Cascades[cascadeIndex].viewProjection * vec4(samplingPos, 1.0);
    vec3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Check bounds
    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }

    // Sample shadow with PCF
    float shadow = SampleShadowPCF(projCoords, cascadeIndex, softness, texelSize, bias);

    // Fade out at max distance
    float fadeStart = u_ShadowParams.z;
    float maxDist = u_ShadowParams.y;
    if (viewDepth > fadeStart) {
        float fadeRatio = (viewDepth - fadeStart) / (maxDist - fadeStart);
        shadow = mix(shadow, 1.0, fadeRatio);
    }

    return shadow;
}

float CalculateSpotShadow(int spotIndex, vec3 worldPos, vec3 normal) {
    if (spotIndex < 0 || spotIndex >= u_ShadowCounts.x) {
        return 1.0;
    }

    SpotShadowData shadow = u_SpotShadows[spotIndex];

    if (shadow.params.w < 0.5) {
        return 1.0;  // Shadow disabled for this light
    }

    float bias = shadow.params.x;
    float normalBias = shadow.params.y;
    float softness = shadow.params.z;

    // Apply normal offset bias
    vec3 samplingPos = worldPos + normal * normalBias;

    // Transform to light space
    vec4 shadowPos = shadow.viewProjection * vec4(samplingPos, 1.0);
    vec3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Check bounds
    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }

    // Transform UV to atlas tile coordinates
    vec2 atlasUV = projCoords.xy * shadow.atlasScaleOffset.xy + shadow.atlasScaleOffset.zw;

    // PCF sampling in atlas
    float result = 0.0;
    float texelSize = shadow.atlasScaleOffset.x / 512.0;  // Approximate texel size

    for (int i = 0; i < PCF_SAMPLES; i++) {
        vec2 offset = POISSON_DISK[i] * softness * texelSize;
        float closestDepth = texture(u_SpotShadowAtlas, atlasUV + offset).r;
        result += (projCoords.z - bias) > closestDepth ? 0.0 : 1.0;
    }

    return result / float(PCF_SAMPLES);
}

// ============================================================================
// Lighting Calculations
// ============================================================================

vec3 CalculatePBR(vec3 L, vec3 V, vec3 N, vec3 lightColor, vec3 albedo,
                  float metallic, float roughness, vec3 F0) {
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * lightColor * NdotL;
}

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 worldPos, vec3 V, vec3 N,
                                vec3 albedo, float metallic, float roughness, vec3 F0,
                                float viewDepth, bool castsShadow) {
    vec3 L = normalize(-light.direction.xyz);
    vec3 lightColor = light.colorIntensity.rgb * light.colorIntensity.a;

    vec3 lighting = CalculatePBR(L, V, N, lightColor, albedo, metallic, roughness, F0);

    // Apply shadow
    if (castsShadow && u_ShadowsEnabled) {
        float shadow = CalculateCSMShadow(worldPos, N, viewDepth);
        lighting *= shadow;
    }

    return lighting;
}

vec3 CalculatePointLight(PointLight light, vec3 worldPos, vec3 V, vec3 N,
                          vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 L = light.position.xyz - worldPos;
    float distance = length(L);

    float radius = light.position.w;
    if (distance > radius) return vec3(0.0);

    L = normalize(L);

    float attenuation = 1.0 / (light.attenuation.x +
                               light.attenuation.y * distance +
                               light.attenuation.z * distance * distance);

    float smoothFalloff = clamp(1.0 - distance / radius, 0.0, 1.0);
    smoothFalloff *= smoothFalloff;
    attenuation *= smoothFalloff;

    vec3 lightColor = light.colorIntensity.rgb * light.colorIntensity.a * attenuation;

    return CalculatePBR(L, V, N, lightColor, albedo, metallic, roughness, F0);
}

vec3 CalculateSpotLight(SpotLight light, vec3 worldPos, vec3 V, vec3 N,
                         vec3 albedo, float metallic, float roughness, vec3 F0,
                         int shadowIndex) {
    vec3 L = light.position.xyz - worldPos;
    float distance = length(L);

    float range = light.position.w;
    if (distance > range) return vec3(0.0);

    L = normalize(L);

    float theta = dot(L, normalize(-light.direction.xyz));
    float epsilon = light.cutoffAtten.x - light.cutoffAtten.y;
    float intensity = clamp((theta - light.cutoffAtten.y) / epsilon, 0.0, 1.0);

    if (intensity <= 0.0) return vec3(0.0);

    float attenuation = 1.0 / (1.0 + light.cutoffAtten.z * distance +
                               light.cutoffAtten.w * distance * distance);

    vec3 lightColor = light.colorIntensity.rgb * light.colorIntensity.a * attenuation * intensity;

    vec3 lighting = CalculatePBR(L, V, N, lightColor, albedo, metallic, roughness, F0);

    // Apply spot shadow if available
    if (u_ShadowsEnabled && shadowIndex >= 0) {
        float shadow = CalculateSpotShadow(shadowIndex, worldPos, N);
        lighting *= shadow;
    }

    return lighting;
}

// ============================================================================
// Main
// ============================================================================

void main() {
    vec4 gPosDepth = texture(u_GPosition, v_TexCoords);
    vec4 gNormalMetallic = texture(u_GNormal, v_TexCoords);
    vec4 gAlbedoRoughness = texture(u_GAlbedo, v_TexCoords);
    vec4 gEmissionAO = texture(u_GEmission, v_TexCoords);

    vec3 worldPos = gPosDepth.rgb;
    vec3 normal = normalize(gNormalMetallic.rgb * 2.0 - 1.0);
    vec3 albedo = gAlbedoRoughness.rgb;
    float metallic = gNormalMetallic.a;
    float roughness = gAlbedoRoughness.a;
    vec3 emission = gEmissionAO.rgb;
    float ao = gEmissionAO.a;

    vec3 V = normalize(u_CameraPos - worldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    // Use linear depth from G-Buffer for cascade selection
    float viewDepth = gPosDepth.a;

    for (int i = 0; i < u_DirectionalLightCount; ++i) {
        // First directional light casts shadows (typically the sun)
        bool castsShadow = (i == 0);
        Lo += CalculateDirectionalLight(u_DirectionalLights[i], worldPos, V, normal,
                                         albedo, metallic, roughness, F0,
                                         viewDepth, castsShadow);
    }

    for (int i = 0; i < u_PointLightCount; ++i) {
        Lo += CalculatePointLight(u_PointLights[i], worldPos, V, normal,
                                   albedo, metallic, roughness, F0);
    }

    for (int i = 0; i < u_SpotLightCount; ++i) {
        // Map spotlight index to shadow index (assumes shadow-casting lights come first)
        int shadowIndex = (i < u_ShadowCounts.x) ? i : -1;
        Lo += CalculateSpotLight(u_SpotLights[i], worldPos, V, normal,
                                  albedo, metallic, roughness, F0, shadowIndex);
    }

    vec3 ambient = u_AmbientLight.rgb * u_AmbientLight.a * albedo * ao;

    vec3 color = ambient + Lo + emission;

    FragColor = vec4(color, 1.0);
}
