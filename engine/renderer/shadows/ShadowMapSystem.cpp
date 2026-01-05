#include "renderer/shadows/ShadowMapSystem.hpp"
#include "ecs/Components/Transform.hpp"
#include "ecs/Components/Renderable.hpp"
#include "ecs/Components/LightComponents.hpp"
#include "renderer/opengl/GLVertexArray.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

ShadowMapSystem::ShadowMapSystem() {
    m_ShadowCasters.reserve(1000);
    m_SpotShadowData.reserve(MAX_SHADOW_CASTING_SPOT);
}

ShadowMapSystem::~ShadowMapSystem() {
    if (m_ShadowDataUBO) {
        glDeleteBuffers(1, &m_ShadowDataUBO);
        m_ShadowDataUBO = 0;
    }
}

void ShadowMapSystem::OnCreate(entt::registry& registry) {
    (void)registry;

    m_CSM = CreateScope<CascadedShadowMap>(m_Settings.CascadeResolution);
    m_SpotAtlas = CreateScope<ShadowAtlas>(m_Settings.SpotShadowAtlasSize);

    LoadShaders();
    CreateUBOs();

    m_Initialized = true;

    LOG_CORE_INFO("ShadowMapSystem initialized (CSM {}x{}, {} cascades, SpotAtlas {}x{})",
                   m_Settings.CascadeResolution, m_Settings.CascadeResolution,
                   CSM_CASCADE_COUNT, m_Settings.SpotShadowAtlasSize, m_Settings.SpotShadowAtlasSize);
}

void ShadowMapSystem::OnDestroy(entt::registry& registry) {
    (void)registry;
    m_Initialized = false;
}

void ShadowMapSystem::OnUpdate(entt::registry& registry, f32 deltaTime) {
    (void)deltaTime;

    if (!m_Initialized || !m_Camera || !m_Settings.Enabled) return;

    m_Stats = {};
    m_FrameNumber++;

    // Begin frame for atlas LRU tracking
    if (m_SpotAtlas) {
        m_SpotAtlas->BeginFrame(m_FrameNumber);
    }

    // Gather all shadow-casting entities
    GatherShadowCasters(registry);

    // Render directional light shadows (CSM)
    RenderDirectionalShadows(registry);

    // Render spot light shadows
    RenderSpotShadows(registry);

    // Upload shadow data to GPU
    UploadShadowData();
}

void ShadowMapSystem::OnReload() {
    LoadShaders();
    LOG_CORE_INFO("Shadow shaders reloaded");
}

void ShadowMapSystem::GatherShadowCasters(entt::registry& registry) {
    m_ShadowCasters.clear();

    auto view = registry.view<Transform, MeshComponent, Renderable>();
    for (auto entity : view) {
        auto& renderable = view.get<Renderable>(entity);

        // Only gather entities that cast shadows and are visible
        if (!renderable.CastShadows || !renderable.Visible) continue;

        auto& transform = view.get<Transform>(entity);
        auto& mesh = view.get<MeshComponent>(entity);

        if (!mesh.VAO) continue;

        ShadowCasterInfo info;
        info.Entity = entity;
        info.WorldMatrix = transform.WorldMatrix;
        info.WorldBounds = renderable.WorldBounds;
        info.MeshId = mesh.MeshId;
        info.IndexCount = mesh.IndexCount;

        m_ShadowCasters.push_back(info);
    }
}

void ShadowMapSystem::RenderDirectionalShadows(entt::registry& registry) {
    // Find first shadow-casting directional light
    auto dirView = registry.view<DirectionalLightComponent>();
    bool foundLight = false;

    for (auto entity : dirView) {
        auto& light = dirView.get<DirectionalLightComponent>(entity);

        if (!light.Enabled || !light.CastShadows) continue;

        // Store current light parameters
        m_CurrentLightDirection = glm::normalize(light.Direction);
        m_CurrentShadowBias = light.ShadowBias;
        m_CurrentNormalBias = light.NormalBias;
        m_CurrentShadowSoftness = light.ShadowSoftness;

        // Update CSM resolution if different
        if (m_CSM && m_CSM->GetResolution() != light.ShadowResolution) {
            m_CSM->Resize(light.ShadowResolution);
        }

        // Update cascade split lambda
        m_Settings.CascadeSplitLambda = light.CascadeSplitLambda;

        foundLight = true;
        break;
    }

    if (!foundLight || m_ShadowCasters.empty()) {
        // No shadow casting light or no shadow casters
        // Still update CSM with dummy data to avoid stale shadows
        m_CSMData.ShadowParams.w = 0.0f;  // Disabled
        return;
    }

    // Update cascade matrices based on camera and light direction
    m_CSM->Update(*m_Camera,
                  m_CurrentLightDirection,
                  m_Settings.MaxShadowDistance,
                  m_Settings.CascadeSplitLambda);

    // Render each cascade
    m_DepthShader->Bind();

    for (u32 cascade = 0; cascade < CSM_CASCADE_COUNT; ++cascade) {
        const auto& cascadeInfo = m_CSM->GetCascadeInfo(cascade);

        m_CSM->BindForCascade(cascade);

        // Clear the cascade depth buffer
        glClear(GL_DEPTH_BUFFER_BIT);

        m_DepthShader->SetMat4("u_LightViewProj", cascadeInfo.ViewProjectionMatrix);

        // Render all shadow casters that intersect this cascade's frustum
        for (const auto& caster : m_ShadowCasters) {
            // Frustum cull against cascade bounds
            if (!cascadeInfo.WorldBounds.Intersects(caster.WorldBounds)) continue;

            m_DepthShader->SetMat4("u_Model", caster.WorldMatrix);

            // Get mesh VAO from registry
            auto& mesh = registry.get<MeshComponent>(caster.Entity);
            mesh.VAO->Bind();
            glDrawElements(GL_TRIANGLES, mesh.IndexCount, GL_UNSIGNED_INT, nullptr);

            m_Stats.ShadowCastersRendered++;
        }

        m_Stats.CascadesRendered++;
    }

    m_CSM->Unbind();
}

void ShadowMapSystem::RenderSpotShadows(entt::registry& registry) {
    if (!m_SpotAtlas || m_ShadowCasters.empty()) return;

    m_SpotShadowData.clear();
    m_SpotShadowCount = 0;

    // Find all shadow-casting spot lights
    auto spotView = registry.view<Transform, SpotLightComponent>();

    m_SpotDepthShader->Bind();

    for (auto entity : spotView) {
        if (m_SpotShadowCount >= MAX_SHADOW_CASTING_SPOT) break;

        auto& transform = spotView.get<Transform>(entity);
        auto& light = spotView.get<SpotLightComponent>(entity);

        if (!light.Enabled || !light.CastShadows) continue;

        // Allocate tile in atlas
        i32 tileIndex = m_SpotAtlas->AllocateTile(512, static_cast<i32>(m_SpotShadowCount));
        if (tileIndex < 0) continue;

        // Get light position and direction
        glm::vec3 lightPos = transform.GetWorldPosition();
        glm::vec3 lightDir = glm::normalize(light.Direction);

        // Calculate view matrix (look from light position in light direction)
        glm::vec3 up = glm::abs(lightDir.y) < 0.999f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, up);

        // Calculate projection matrix based on outer cutoff angle
        f32 fov = light.OuterCutOff * 2.0f;
        f32 nearPlane = 0.1f;
        f32 farPlane = light.Range;
        glm::mat4 lightProj = glm::perspective(fov, 1.0f, nearPlane, farPlane);

        glm::mat4 lightViewProj = lightProj * lightView;

        // Bind atlas tile for rendering
        m_SpotAtlas->BindForTile(tileIndex);

        // Clear the tile
        glClear(GL_DEPTH_BUFFER_BIT);

        m_SpotDepthShader->SetMat4("u_LightViewProj", lightViewProj);

        // Render shadow casters
        for (const auto& caster : m_ShadowCasters) {
            // Simple distance culling
            glm::vec3 casterCenter = caster.WorldBounds.GetCenter();
            f32 distance = glm::length(casterCenter - lightPos);
            if (distance > light.Range * 1.5f) continue;

            m_SpotDepthShader->SetMat4("u_Model", caster.WorldMatrix);

            auto& mesh = registry.get<MeshComponent>(caster.Entity);
            mesh.VAO->Bind();
            glDrawElements(GL_TRIANGLES, mesh.IndexCount, GL_UNSIGNED_INT, nullptr);
        }

        // Store GPU data
        GPUSpotShadowData gpuData;
        gpuData.ViewProjection = lightViewProj;
        gpuData.AtlasScaleOffset = m_SpotAtlas->GetTileScaleOffset(tileIndex);
        gpuData.ShadowParams = glm::vec4(0.001f, 0.01f, 1.0f, 1.0f);  // bias, normalBias, softness, enabled

        m_SpotShadowData.push_back(gpuData);
        m_SpotShadowCount++;
        m_Stats.SpotShadowsRendered++;
    }

    m_SpotAtlas->Unbind();
}

void ShadowMapSystem::UploadShadowData() {
    // Fill CSM GPU data structure
    m_CSM->FillGPUData(m_CSMData,
                        m_CurrentShadowSoftness,
                        m_CurrentShadowBias,
                        m_CurrentNormalBias);

    // Upload CSM data to UBO
    glNamedBufferSubData(m_ShadowDataUBO, 0, sizeof(GPUCascadedShadowData), &m_CSMData);

    // Upload spot shadow data
    if (m_SpotShadowCount > 0) {
        size_t spotOffset = sizeof(GPUCascadedShadowData);
        size_t spotSize = m_SpotShadowCount * sizeof(GPUSpotShadowData);
        glNamedBufferSubData(m_ShadowDataUBO, spotOffset, spotSize, m_SpotShadowData.data());
    }

    // Upload shadow counts
    glm::ivec4 counts(static_cast<i32>(m_SpotShadowCount), 0, 0, 0);
    size_t countsOffset = sizeof(GPUCascadedShadowData) + MAX_SHADOW_CASTING_SPOT * sizeof(GPUSpotShadowData);
    glNamedBufferSubData(m_ShadowDataUBO, countsOffset, sizeof(glm::ivec4), &counts);
}

void ShadowMapSystem::LoadShaders() {
    m_DepthShader = CreateRef<Shader>("assets/shaders/shadows/depth_csm.glsl");
    m_SpotDepthShader = CreateRef<Shader>("assets/shaders/shadows/depth_spot.glsl");

    LOG_CORE_DEBUG("Shadow depth shaders loaded (CSM + Spot)");
}

void ShadowMapSystem::CreateUBOs() {
    // Calculate total UBO size: CSM + Spot shadows + counts
    size_t uboSize = sizeof(GPUCascadedShadowData) +
                     MAX_SHADOW_CASTING_SPOT * sizeof(GPUSpotShadowData) +
                     sizeof(glm::ivec4);

    glCreateBuffers(1, &m_ShadowDataUBO);
    glNamedBufferStorage(m_ShadowDataUBO, uboSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

    LOG_CORE_DEBUG("Shadow UBO created (size: {} bytes, CSM + {} spot slots)",
                   uboSize, MAX_SHADOW_CASTING_SPOT);
}

void ShadowMapSystem::BindCSMTexture(u32 slot) const {
    if (m_CSM) {
        m_CSM->BindTexture(slot);
    }
}

void ShadowMapSystem::BindSpotAtlasTexture(u32 slot) const {
    if (m_SpotAtlas) {
        m_SpotAtlas->BindTexture(slot);
    }
}

} // namespace Engine
