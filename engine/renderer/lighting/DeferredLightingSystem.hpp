#pragma once

#include "ecs/System.hpp"
#include "renderer/pipeline/GBuffer.hpp"
#include "renderer/pipeline/Framebuffer.hpp"
#include "renderer/opengl/GLShader.hpp"
#include "renderer/opengl/GLVertexArray.hpp"
#include "camera/Camera.hpp"
#include <glm/glm.hpp>

namespace Engine {

struct GPUDirectionalLight {
    glm::vec4 Direction;
    glm::vec4 ColorIntensity;
};

struct GPUPointLight {
    glm::vec4 Position;
    glm::vec4 ColorIntensity;
    glm::vec4 Attenuation;
};

struct GPUSpotLight {
    glm::vec4 Position;
    glm::vec4 Direction;
    glm::vec4 ColorIntensity;
    glm::vec4 CutoffAttenuation;
};

class DeferredLightingSystem : public ISystem {
public:
    DeferredLightingSystem();
    ~DeferredLightingSystem();

    DEFINE_SYSTEM(DeferredLightingSystem, Render, 50);

    void OnCreate(entt::registry& registry) override;
    void OnDestroy(entt::registry& registry) override;
    void OnUpdate(entt::registry& registry, f32 deltaTime) override;
    void OnReload() override;

    void SetCamera(Camera* camera) { m_Camera = camera; }
    void Resize(u32 width, u32 height);

    // Shadow system integration (forward declared at namespace level)
    void SetShadowSystem(class ShadowMapSystem* shadowSystem) { m_ShadowSystem = shadowSystem; }

    GBuffer& GetGBuffer() { return *m_GBuffer; }
    const GBuffer& GetGBuffer() const { return *m_GBuffer; }
    Framebuffer& GetLightingBuffer() { return *m_LightingBuffer; }
    const Framebuffer& GetLightingBuffer() const { return *m_LightingBuffer; }

    u32 GetLightingTextureID() const;

    static constexpr u32 MaxDirectionalLights = 4;
    static constexpr u32 MaxPointLights = 256;
    static constexpr u32 MaxSpotLights = 64;

    struct Stats {
        u32 DirectionalLightCount = 0;
        u32 PointLightCount = 0;
        u32 SpotLightCount = 0;
        u32 EntitiesRendered = 0;
    };

    const Stats& GetStats() const { return m_Stats; }

private:
    void GeometryPass(entt::registry& registry);
    void LightingPass(entt::registry& registry);

    void GatherLights(entt::registry& registry);
    void UploadLightData();

    void CreateScreenQuad();
    void CreateLightUBOs();
    void LoadShaders();

private:
    Camera* m_Camera = nullptr;
    class ShadowMapSystem* m_ShadowSystem = nullptr;

    Scope<GBuffer> m_GBuffer;
    Scope<Framebuffer> m_LightingBuffer;

    Ref<Shader> m_GeometryShader;
    Ref<Shader> m_LightingShader;

    Ref<VertexArray> m_ScreenQuadVAO;

    Vector<GPUDirectionalLight> m_DirectionalLights;
    Vector<GPUPointLight> m_PointLights;
    Vector<GPUSpotLight> m_SpotLights;
    glm::vec4 m_AmbientLight{0.03f, 0.03f, 0.03f, 1.0f};

    u32 m_DirectionalLightUBO = 0;
    u32 m_PointLightUBO = 0;
    u32 m_SpotLightUBO = 0;

    Stats m_Stats;
    u32 m_Width = 1280;
    u32 m_Height = 720;
    bool m_Initialized = false;
};

} // namespace Engine
