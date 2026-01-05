#pragma once

#include "ecs/Component.hpp"
#include "core/Types.hpp"
#include "math/AABB.hpp"
#include <glm/glm.hpp>

namespace Engine {

// Forward declarations for renderer types
class Shader;
class VertexArray;
class Material;

// Mesh reference component
struct MeshComponent {
    Ref<VertexArray> VAO;
    u32 IndexCount = 0;
    u32 VertexCount = 0;

    // Bounding volumes for culling
    AABB LocalBounds;
    BoundingSphere LocalSphere;

    // Mesh identification
    String MeshName;
    u32 MeshId = 0;
};

// Material reference component
struct MaterialComponent {
    Ref<Shader> ShaderRef;
    // Future: Ref<Material> MaterialRef;

    // Basic material properties (will be expanded)
    glm::vec4 BaseColor{1.0f, 1.0f, 1.0f, 1.0f};
    f32 Metallic = 0.0f;
    f32 Roughness = 0.5f;

    // Material identification
    String MaterialName;
    u32 MaterialId = 0;
};

// Combined renderable component for entities that should be drawn
struct Renderable {
    // Mesh and material indices (for batching)
    u32 MeshId = 0;
    u32 MaterialId = 0;

    // Render state
    bool Visible = true;
    bool CastShadows = true;
    bool ReceiveShadows = true;

    // LOD
    u8 LODLevel = 0;
    u8 MaxLODLevel = 4;

    // Render layer (for filtering)
    u32 RenderLayer = 1;  // Bitmask

    // Sort key for batching (computed at runtime)
    u32 SortKey = 0;

    // Computed bounds (in world space, updated by systems)
    AABB WorldBounds;
    BoundingSphere WorldSphere;

    // Visibility flags (set by culling system)
    bool InFrustum = true;
    bool OcclusionCulled = false;
};

// Tag for entities that need culling check
struct NeedsCulling {};

// Tag for static geometry (doesn't move, can be optimized)
struct StaticGeometry {};

// Tag for dynamic geometry (moves, needs bounds update)
struct DynamicGeometry {};

// Instance data for batched rendering
struct InstanceData {
    glm::mat4 Transform;
    glm::vec4 Color{1.0f};
    u32 EntityId = 0;  // For picking
    u32 Flags = 0;
};

// Batch info for instanced rendering
struct BatchInfo {
    u32 MeshId = 0;
    u32 MaterialId = 0;
    u32 StartIndex = 0;
    u32 InstanceCount = 0;
};

// Render statistics component (optional, for debugging)
struct RenderStats {
    u32 TrianglesRendered = 0;
    u32 DrawCalls = 0;
    u32 EntitiesRendered = 0;
    u32 EntitiesCulled = 0;
    f32 RenderTime = 0.0f;
};

} // namespace Engine

// Reflection registrations
REFLECT_COMPONENT(Engine::MeshComponent,
    .data<&Engine::MeshComponent::IndexCount>("IndexCount"_hs)
    .data<&Engine::MeshComponent::VertexCount>("VertexCount"_hs)
    .data<&Engine::MeshComponent::MeshName>("MeshName"_hs)
);

REFLECT_COMPONENT(Engine::MaterialComponent,
    .data<&Engine::MaterialComponent::BaseColor>("BaseColor"_hs)
    .data<&Engine::MaterialComponent::Metallic>("Metallic"_hs)
    .data<&Engine::MaterialComponent::Roughness>("Roughness"_hs)
    .data<&Engine::MaterialComponent::MaterialName>("MaterialName"_hs)
);

REFLECT_COMPONENT(Engine::Renderable,
    .data<&Engine::Renderable::Visible>("Visible"_hs)
    .data<&Engine::Renderable::CastShadows>("CastShadows"_hs)
    .data<&Engine::Renderable::ReceiveShadows>("ReceiveShadows"_hs)
    .data<&Engine::Renderable::LODLevel>("LODLevel"_hs)
    .data<&Engine::Renderable::RenderLayer>("RenderLayer"_hs)
);

REFLECT_TAG(Engine::NeedsCulling);
REFLECT_TAG(Engine::StaticGeometry);
REFLECT_TAG(Engine::DynamicGeometry);
