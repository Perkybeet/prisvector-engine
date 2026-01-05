// ============================================================================
// Shadow Sampling Functions
// Include this file in your lighting shader for shadow calculations
// ============================================================================

#ifndef SHADOW_COMMON_GLSL
#define SHADOW_COMMON_GLSL

// ============================================================================
// Constants
// ============================================================================
#define CSM_CASCADE_COUNT 4
#define PCF_SAMPLES 16

// Poisson disk samples for PCF soft shadows
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

// ============================================================================
// Cascade Selection
// ============================================================================

// Select the appropriate cascade based on view-space depth
int GetCascadeIndex(float viewDepth, vec4 cascadeSplits) {
    int cascadeIndex = 0;
    for (int i = 0; i < CSM_CASCADE_COUNT; i++) {
        if (viewDepth > cascadeSplits[i]) {
            cascadeIndex = i + 1;
        }
    }
    return min(cascadeIndex, CSM_CASCADE_COUNT - 1);
}

// ============================================================================
// PCF Shadow Sampling
// ============================================================================

// Sample shadow map with hardware PCF (requires sampler2DArrayShadow)
float SampleShadowPCF(sampler2DArrayShadow shadowMap, vec3 shadowCoord,
                       int cascadeIndex, float softness, float texelSize) {
    float shadow = 0.0;

    for (int i = 0; i < PCF_SAMPLES; i++) {
        vec2 offset = POISSON_DISK[i] * softness * texelSize;
        vec4 coords = vec4(shadowCoord.xy + offset, float(cascadeIndex), shadowCoord.z);
        shadow += texture(shadowMap, coords);
    }

    return shadow / float(PCF_SAMPLES);
}

// Sample shadow map without hardware comparison (requires sampler2DArray)
float SampleShadowPCFManual(sampler2DArray shadowMap, vec3 shadowCoord,
                             int cascadeIndex, float bias, float softness, float texelSize) {
    float shadow = 0.0;
    float currentDepth = shadowCoord.z;

    for (int i = 0; i < PCF_SAMPLES; i++) {
        vec2 offset = POISSON_DISK[i] * softness * texelSize;
        float closestDepth = texture(shadowMap, vec3(shadowCoord.xy + offset, float(cascadeIndex))).r;
        shadow += (currentDepth - bias) > closestDepth ? 0.0 : 1.0;
    }

    return shadow / float(PCF_SAMPLES);
}

// ============================================================================
// CSM Shadow Calculation
// ============================================================================

// Calculate CSM shadow factor for a world position
// Returns 1.0 for fully lit, 0.0 for fully shadowed
float CalculateCSMShadow(
    vec3 worldPos,
    vec3 normal,
    float viewDepth,
    mat4 csmViewProj[CSM_CASCADE_COUNT],
    vec4 cascadeSplits,
    sampler2DArray shadowMap,
    float bias,
    float normalBias,
    float softness,
    float texelSize,
    float maxShadowDistance,
    float fadeDistance
) {
    // Check if beyond max shadow distance
    if (viewDepth > maxShadowDistance) {
        return 1.0;
    }

    // Select cascade
    int cascadeIndex = GetCascadeIndex(viewDepth, cascadeSplits);

    // Apply normal offset bias to reduce shadow acne on surfaces facing the light
    vec3 samplingPos = worldPos + normal * normalBias;

    // Transform to light space
    vec4 shadowPos = csmViewProj[cascadeIndex] * vec4(samplingPos, 1.0);
    vec3 projCoords = shadowPos.xyz / shadowPos.w;

    // Transform from [-1,1] to [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // Check if outside shadow map bounds
    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }

    // Sample shadow with PCF
    float shadow = SampleShadowPCFManual(shadowMap, projCoords, cascadeIndex,
                                          bias, softness, texelSize);

    // Fade out shadows at max distance
    float fadeStart = maxShadowDistance - fadeDistance;
    if (viewDepth > fadeStart) {
        float fadeRatio = (viewDepth - fadeStart) / fadeDistance;
        shadow = mix(shadow, 1.0, fadeRatio);
    }

    // Optional: Blend between cascades to reduce visible seams
    // This can be enabled for smoother transitions at cascade boundaries
    #ifdef SHADOW_CASCADE_BLEND
    float cascadeTransition = 0.1;
    float cascadeSplit = cascadeSplits[cascadeIndex];
    float prevCascadeSplit = cascadeIndex > 0 ? cascadeSplits[cascadeIndex - 1] : 0.0;
    float blendRegion = (cascadeSplit - prevCascadeSplit) * cascadeTransition;

    if (cascadeIndex < CSM_CASCADE_COUNT - 1 &&
        viewDepth > cascadeSplit - blendRegion) {
        // Sample next cascade
        vec4 nextShadowPos = csmViewProj[cascadeIndex + 1] * vec4(samplingPos, 1.0);
        vec3 nextProjCoords = (nextShadowPos.xyz / nextShadowPos.w) * 0.5 + 0.5;
        float nextShadow = SampleShadowPCFManual(shadowMap, nextProjCoords,
                                                  cascadeIndex + 1, bias, softness, texelSize);

        float blendFactor = (viewDepth - (cascadeSplit - blendRegion)) / blendRegion;
        shadow = mix(shadow, nextShadow, blendFactor);
    }
    #endif

    return shadow;
}

// ============================================================================
// Debug Visualization
// ============================================================================

// Get cascade color for debug visualization
vec3 GetCascadeDebugColor(int cascadeIndex) {
    const vec3 cascadeColors[4] = vec3[](
        vec3(1.0, 0.0, 0.0),  // Red - cascade 0 (closest)
        vec3(0.0, 1.0, 0.0),  // Green - cascade 1
        vec3(0.0, 0.0, 1.0),  // Blue - cascade 2
        vec3(1.0, 1.0, 0.0)   // Yellow - cascade 3 (farthest)
    );
    return cascadeColors[clamp(cascadeIndex, 0, 3)];
}

#endif // SHADOW_COMMON_GLSL
