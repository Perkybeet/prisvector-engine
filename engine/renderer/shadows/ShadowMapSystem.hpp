#pragma once

#include "ecs/System.hpp"
#include "renderer/shadows/ShadowTypes.hpp"
#include "renderer/shadows/CascadedShadowMap.hpp"
#include "renderer/shadows/ShadowAtlas.hpp"
#include "renderer/opengl/GLShader.hpp"
#include "camera/Camera.hpp"

namespace Engine {

class ShadowMapSystem : public ISystem {
public:
    ShadowMapSystem();
    ~ShadowMapSystem();

    DEFINE_SYSTEM(ShadowMapSystem, PreRender, 10);

    void OnCreate(entt::registry& registry) override;
    void OnDestroy(entt::registry& registry) override;
    void OnUpdate(entt::registry& registry, f32 deltaTime) override;
    void OnReload() override;

    // Camera reference (set by application, same as DeferredLightingSystem)
    void SetCamera(Camera* camera) { m_Camera = camera; }

    // Access for lighting pass integration
    CascadedShadowMap* GetCSM() { return m_CSM.get(); }
    const CascadedShadowMap* GetCSM() const { return m_CSM.get(); }

    ShadowAtlas* GetSpotAtlas() { return m_SpotAtlas.get(); }
    const ShadowAtlas* GetSpotAtlas() const { return m_SpotAtlas.get(); }

    // Settings
    ShadowSettings& GetSettings() { return m_Settings; }
    const ShadowSettings& GetSettings() const { return m_Settings; }

    // GPU data for binding in lighting pass
    u32 GetShadowDataUBO() const { return m_ShadowDataUBO; }

    // Texture binding for lighting pass
    void BindCSMTexture(u32 slot) const;
    void BindSpotAtlasTexture(u32 slot) const;

    // Statistics
    struct Stats {
        u32 ShadowCastersRendered = 0;
        u32 CascadesRendered = 0;
        u32 SpotShadowsRendered = 0;
        f32 ShadowPassTimeMs = 0.0f;
    };
    const Stats& GetStats() const { return m_Stats; }

private:
    void GatherShadowCasters(entt::registry& registry);
    void RenderDirectionalShadows(entt::registry& registry);
    void RenderSpotShadows(entt::registry& registry);
    void UploadShadowData();
    void LoadShaders();
    void CreateUBOs();

private:
    Camera* m_Camera = nullptr;
    ShadowSettings m_Settings;

    Scope<CascadedShadowMap> m_CSM;
    Scope<ShadowAtlas> m_SpotAtlas;

    Ref<Shader> m_DepthShader;
    Ref<Shader> m_SpotDepthShader;

    Vector<ShadowCasterInfo> m_ShadowCasters;

    // GPU data
    u32 m_ShadowDataUBO = 0;
    GPUCascadedShadowData m_CSMData;
    Vector<GPUSpotShadowData> m_SpotShadowData;
    u32 m_SpotShadowCount = 0;

    // Current directional light shadow info
    glm::vec3 m_CurrentLightDirection{0.0f, -1.0f, 0.0f};
    f32 m_CurrentShadowBias = 0.0005f;
    f32 m_CurrentNormalBias = 0.02f;
    f32 m_CurrentShadowSoftness = 1.0f;

    Stats m_Stats;
    u32 m_FrameNumber = 0;
    bool m_Initialized = false;
};

} // namespace Engine
