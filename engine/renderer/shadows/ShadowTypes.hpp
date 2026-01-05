#pragma once

#include "core/Types.hpp"
#include "math/AABB.hpp"
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <array>

namespace Engine {

// ============================================================================
// Shadow System Constants
// ============================================================================
constexpr u32 CSM_CASCADE_COUNT = 4;
constexpr u32 MAX_SHADOW_CASTING_DIRECTIONAL = 1;
constexpr u32 MAX_SHADOW_CASTING_SPOT = 16;
constexpr u32 MAX_SHADOW_CASTING_POINT = 8;

constexpr u32 DEFAULT_CSM_RESOLUTION = 2048;
constexpr u32 DEFAULT_SPOT_SHADOW_ATLAS_SIZE = 4096;
constexpr u32 DEFAULT_POINT_SHADOW_ATLAS_SIZE = 2048;

// UBO binding point for shadow data (after light UBOs at 0, 1, 2)
constexpr u32 SHADOW_UBO_BINDING = 3;

// ============================================================================
// Shadow Settings (runtime configurable)
// ============================================================================
struct ShadowSettings {
    bool Enabled = true;

    // CSM Settings
    u32 CascadeResolution = DEFAULT_CSM_RESOLUTION;
    f32 CascadeSplitLambda = 0.75f;     // 0 = linear, 1 = logarithmic
    f32 MaxShadowDistance = 200.0f;
    f32 CascadeFadeRange = 0.1f;        // Blend between cascades

    // PCF Settings
    u32 PCFSamples = 16;
    bool UsePCSS = false;               // Percentage-closer soft shadows
    f32 LightSize = 0.02f;              // For PCSS penumbra calculation

    // Atlas sizes
    u32 SpotShadowAtlasSize = DEFAULT_SPOT_SHADOW_ATLAS_SIZE;
    u32 PointShadowAtlasSize = DEFAULT_POINT_SHADOW_ATLAS_SIZE;
};

// ============================================================================
// GPU-aligned structures for UBO upload (std140 layout)
// ============================================================================

// Per-cascade data for CSM
struct alignas(16) GPUCascadeData {
    glm::mat4 ViewProjection;           // 64 bytes - Light space transform
    glm::vec4 SplitDepthBias;           // 16 bytes - x=splitDepth, y=texelSize, z=bias, w=normalBias
};  // Total: 80 bytes per cascade

// CSM UBO structure (matches shader layout)
struct GPUCascadedShadowData {
    GPUCascadeData Cascades[CSM_CASCADE_COUNT];   // 4 * 80 = 320 bytes
    glm::vec4 CascadeSplitDepths;                 // 16 bytes - Far plane of each cascade
    glm::vec4 ShadowParams;                       // 16 bytes - x=softness, y=maxDist, z=fadeStart, w=enabled
};  // Total: 352 bytes

// Spot light shadow data for GPU
struct alignas(16) GPUSpotShadowData {
    glm::mat4 ViewProjection;           // 64 bytes - Light space transform
    glm::vec4 AtlasScaleOffset;         // 16 bytes - xy=scale, zw=offset in atlas UV
    glm::vec4 ShadowParams;             // 16 bytes - x=bias, y=normalBias, z=softness, w=enabled
};  // Total: 96 bytes

// Point light shadow data for GPU (cubemap shadows)
struct alignas(16) GPUPointShadowData {
    glm::vec4 PositionFarPlane;         // 16 bytes - xyz=position, w=farPlane
    glm::vec4 ShadowParams;             // 16 bytes - x=bias, y=normalBias, z=softness, w=atlasLayer
};  // Total: 32 bytes

// ============================================================================
// Complete Shadow UBO structure
// ============================================================================
struct GPUShadowUBO {
    // CSM data (352 bytes)
    GPUCascadedShadowData CSM;

    // Spot shadow data (16 * 96 = 1536 bytes)
    GPUSpotShadowData SpotShadows[MAX_SHADOW_CASTING_SPOT];

    // Counts and padding
    glm::ivec4 ShadowCounts;            // x=spotCount, y=pointCount, z=reserved, w=reserved
};

// ============================================================================
// CPU-side runtime structures
// ============================================================================

// Information about a single cascade
struct CascadeInfo {
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewProjectionMatrix;
    f32 SplitNear;
    f32 SplitFar;
    AABB WorldBounds;                   // For frustum culling shadow casters
};

// Information about a shadow-casting entity
struct ShadowCasterInfo {
    entt::entity Entity;
    glm::mat4 WorldMatrix;
    AABB WorldBounds;
    u32 MeshId;
    u32 IndexCount;
};

// Shadow atlas tile information
struct ShadowAtlasTile {
    u32 X = 0;
    u32 Y = 0;
    u32 Size = 0;
    i32 LightIndex = -1;                // -1 = free
    u32 LastUsedFrame = 0;              // For LRU eviction

    bool IsFree() const { return LightIndex < 0; }

    glm::vec4 GetScaleOffset(u32 atlasSize) const {
        f32 scale = static_cast<f32>(Size) / static_cast<f32>(atlasSize);
        f32 offsetX = static_cast<f32>(X) / static_cast<f32>(atlasSize);
        f32 offsetY = static_cast<f32>(Y) / static_cast<f32>(atlasSize);
        return glm::vec4(scale, scale, offsetX, offsetY);
    }
};

} // namespace Engine
