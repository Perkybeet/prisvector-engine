#include "renderer/lighting/DeferredLightingSystem.hpp"
#include "renderer/shadows/ShadowMapSystem.hpp"
#include "ecs/Components/Transform.hpp"
#include "ecs/Components/Renderable.hpp"
#include "ecs/Components/LightComponents.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

DeferredLightingSystem::DeferredLightingSystem() {
    m_DirectionalLights.reserve(MaxDirectionalLights);
    m_PointLights.reserve(MaxPointLights);
    m_SpotLights.reserve(MaxSpotLights);
}

DeferredLightingSystem::~DeferredLightingSystem() {
    if (m_DirectionalLightUBO) glDeleteBuffers(1, &m_DirectionalLightUBO);
    if (m_PointLightUBO) glDeleteBuffers(1, &m_PointLightUBO);
    if (m_SpotLightUBO) glDeleteBuffers(1, &m_SpotLightUBO);
}

void DeferredLightingSystem::OnCreate(entt::registry& registry) {
    (void)registry;

    m_GBuffer = CreateScope<GBuffer>(m_Width, m_Height);

    FramebufferSpecification lightingSpec;
    lightingSpec.Width = m_Width;
    lightingSpec.Height = m_Height;
    lightingSpec.Attachments = {
        FramebufferTextureFormat::RGBA16F,
        FramebufferTextureFormat::Depth24Stencil8
    };
    m_LightingBuffer = CreateScope<Framebuffer>(lightingSpec);

    LoadShaders();
    CreateScreenQuad();
    CreateLightUBOs();

    m_Initialized = true;

    LOG_CORE_INFO("DeferredLightingSystem initialized ({}x{})", m_Width, m_Height);
}

void DeferredLightingSystem::OnDestroy(entt::registry& registry) {
    (void)registry;
    m_Initialized = false;
}

void DeferredLightingSystem::OnUpdate(entt::registry& registry, f32 deltaTime) {
    (void)deltaTime;

    if (!m_Initialized || !m_Camera) return;

    m_Stats = {};

    GatherLights(registry);

    GeometryPass(registry);

    LightingPass(registry);
}

void DeferredLightingSystem::OnReload() {
    LoadShaders();
}

void DeferredLightingSystem::Resize(u32 width, u32 height) {
    if (width == 0 || height == 0) return;

    m_Width = width;
    m_Height = height;

    if (m_GBuffer) {
        m_GBuffer->Resize(width, height);
    }
    if (m_LightingBuffer) {
        m_LightingBuffer->Resize(width, height);
    }
}

u32 DeferredLightingSystem::GetLightingTextureID() const {
    if (m_LightingBuffer) {
        return m_LightingBuffer->GetColorAttachmentRendererID(0);
    }
    return 0;
}

void DeferredLightingSystem::GeometryPass(entt::registry& registry) {
    m_GBuffer->Bind();
    m_GBuffer->Clear();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_GeometryShader->Bind();

    glm::mat4 viewProj = m_Camera->GetViewProjectionMatrix();
    m_GeometryShader->SetMat4("u_ViewProjection", viewProj);
    m_GeometryShader->SetFloat("u_NearPlane", 0.1f);
    m_GeometryShader->SetFloat("u_FarPlane", 1000.0f);

    auto view = registry.view<Transform, MeshComponent, MaterialComponent, Renderable>();
    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& mesh = view.get<MeshComponent>(entity);
        auto& material = view.get<MaterialComponent>(entity);
        auto& renderable = view.get<Renderable>(entity);

        if (!renderable.Visible || !renderable.InFrustum) continue;
        if (!mesh.VAO) continue;

        glm::mat4 model = transform.WorldMatrix;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        m_GeometryShader->SetMat4("u_Model", model);
        m_GeometryShader->SetMat3("u_NormalMatrix", normalMatrix);

        m_GeometryShader->SetFloat4("u_AlbedoColor", material.BaseColor);
        m_GeometryShader->SetFloat("u_Metallic", material.Metallic);
        m_GeometryShader->SetFloat("u_Roughness", material.Roughness);
        m_GeometryShader->SetFloat("u_AO", 1.0f);
        m_GeometryShader->SetFloat3("u_EmissiveColor", glm::vec3(0.0f));
        m_GeometryShader->SetFloat("u_EmissiveIntensity", 0.0f);
        m_GeometryShader->SetFloat("u_NormalStrength", 1.0f);
        m_GeometryShader->SetFloat2("u_TilingFactor", glm::vec2(1.0f));
        m_GeometryShader->SetInt("u_TextureFlags", 0);

        mesh.VAO->Bind();
        glDrawElements(GL_TRIANGLES, mesh.IndexCount, GL_UNSIGNED_INT, nullptr);

        m_Stats.EntitiesRendered++;
    }

    m_GBuffer->Unbind();
}

void DeferredLightingSystem::LightingPass(entt::registry& registry) {
    (void)registry;

    UploadLightData();

    m_LightingBuffer->Bind();
    // Light gray background for editor
    m_LightingBuffer->Clear(glm::vec4(0.15f, 0.15f, 0.17f, 1.0f), 1.0f);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_LightingShader->Bind();

    m_GBuffer->BindTextures(0);
    m_LightingShader->SetInt("u_GPosition", 0);
    m_LightingShader->SetInt("u_GNormal", 1);
    m_LightingShader->SetInt("u_GAlbedo", 2);
    m_LightingShader->SetInt("u_GEmission", 3);

    m_LightingShader->SetFloat3("u_CameraPos", m_Camera->GetPosition());
    m_LightingShader->SetFloat4("u_AmbientLight", m_AmbientLight);

    m_LightingShader->SetInt("u_DirectionalLightCount", static_cast<i32>(m_Stats.DirectionalLightCount));
    m_LightingShader->SetInt("u_PointLightCount", static_cast<i32>(m_Stats.PointLightCount));
    m_LightingShader->SetInt("u_SpotLightCount", static_cast<i32>(m_Stats.SpotLightCount));

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_DirectionalLightUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_PointLightUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_SpotLightUBO);

    // Bind shadow maps and data
    bool shadowsEnabled = false;
    if (m_ShadowSystem && m_ShadowSystem->GetSettings().Enabled) {
        shadowsEnabled = true;

        // Bind CSM shadow map texture (slot 4)
        m_ShadowSystem->BindCSMTexture(4);
        m_LightingShader->SetInt("u_CSMShadowMap", 4);

        // Bind spot shadow atlas texture (slot 5)
        m_ShadowSystem->BindSpotAtlasTexture(5);
        m_LightingShader->SetInt("u_SpotShadowAtlas", 5);

        // Bind shadow UBO (binding = 3)
        glBindBufferBase(GL_UNIFORM_BUFFER, 3, m_ShadowSystem->GetShadowDataUBO());
    }
    m_LightingShader->SetInt("u_ShadowsEnabled", shadowsEnabled ? 1 : 0);

    m_ScreenQuadVAO->Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    m_LightingBuffer->Unbind();

    glEnable(GL_DEPTH_TEST);
}

void DeferredLightingSystem::GatherLights(entt::registry& registry) {
    m_DirectionalLights.clear();
    m_PointLights.clear();
    m_SpotLights.clear();

    auto ambientView = registry.view<AmbientLightComponent>();
    for (auto entity : ambientView) {
        auto& ambient = ambientView.get<AmbientLightComponent>(entity);
        m_AmbientLight = glm::vec4(ambient.Color, ambient.Intensity);
        break;
    }

    auto dirView = registry.view<DirectionalLightComponent>();
    for (auto entity : dirView) {
        if (m_DirectionalLights.size() >= MaxDirectionalLights) break;

        auto& light = dirView.get<DirectionalLightComponent>(entity);
        if (!light.Enabled) continue;

        GPUDirectionalLight gpuLight;
        gpuLight.Direction = glm::vec4(glm::normalize(light.Direction), 0.0f);
        gpuLight.ColorIntensity = glm::vec4(light.Color, light.Intensity);

        m_DirectionalLights.push_back(gpuLight);
    }

    auto pointView = registry.view<Transform, PointLightComponent>();
    for (auto entity : pointView) {
        if (m_PointLights.size() >= MaxPointLights) break;

        auto& transform = pointView.get<Transform>(entity);
        auto& light = pointView.get<PointLightComponent>(entity);
        if (!light.Enabled) continue;

        GPUPointLight gpuLight;
        gpuLight.Position = glm::vec4(transform.GetWorldPosition(), light.Radius);
        gpuLight.ColorIntensity = glm::vec4(light.Color, light.Intensity);
        gpuLight.Attenuation = glm::vec4(light.Constant, light.Linear, light.Quadratic, 0.0f);

        m_PointLights.push_back(gpuLight);
    }

    auto spotView = registry.view<Transform, SpotLightComponent>();
    for (auto entity : spotView) {
        if (m_SpotLights.size() >= MaxSpotLights) break;

        auto& transform = spotView.get<Transform>(entity);
        auto& light = spotView.get<SpotLightComponent>(entity);
        if (!light.Enabled) continue;

        GPUSpotLight gpuLight;
        gpuLight.Position = glm::vec4(transform.GetWorldPosition(), light.Range);
        gpuLight.Direction = glm::vec4(glm::normalize(light.Direction), 0.0f);
        gpuLight.ColorIntensity = glm::vec4(light.Color, light.Intensity);
        gpuLight.CutoffAttenuation = glm::vec4(
            glm::cos(light.InnerCutOff),
            glm::cos(light.OuterCutOff),
            light.Linear,
            light.Quadratic
        );

        m_SpotLights.push_back(gpuLight);
    }

    m_Stats.DirectionalLightCount = static_cast<u32>(m_DirectionalLights.size());
    m_Stats.PointLightCount = static_cast<u32>(m_PointLights.size());
    m_Stats.SpotLightCount = static_cast<u32>(m_SpotLights.size());
}

void DeferredLightingSystem::UploadLightData() {
    if (!m_DirectionalLights.empty()) {
        glNamedBufferSubData(m_DirectionalLightUBO, 0,
                              m_DirectionalLights.size() * sizeof(GPUDirectionalLight),
                              m_DirectionalLights.data());
    }

    if (!m_PointLights.empty()) {
        glNamedBufferSubData(m_PointLightUBO, 0,
                              m_PointLights.size() * sizeof(GPUPointLight),
                              m_PointLights.data());
    }

    if (!m_SpotLights.empty()) {
        glNamedBufferSubData(m_SpotLightUBO, 0,
                              m_SpotLights.size() * sizeof(GPUSpotLight),
                              m_SpotLights.data());
    }
}

void DeferredLightingSystem::CreateScreenQuad() {
    f32 quadVertices[] = {
        // Position          TexCoords
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };

    u32 quadIndices[] = { 0, 1, 2, 2, 3, 0 };

    m_ScreenQuadVAO = CreateRef<VertexArray>();

    auto vbo = CreateRef<VertexBuffer>(quadVertices, sizeof(quadVertices));
    vbo->SetLayout({
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float2, "a_TexCoords" }
    });

    auto ibo = CreateRef<IndexBuffer>(quadIndices, 6);

    m_ScreenQuadVAO->AddVertexBuffer(vbo);
    m_ScreenQuadVAO->SetIndexBuffer(ibo);
}

void DeferredLightingSystem::CreateLightUBOs() {
    glCreateBuffers(1, &m_DirectionalLightUBO);
    glNamedBufferStorage(m_DirectionalLightUBO,
                          MaxDirectionalLights * sizeof(GPUDirectionalLight),
                          nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &m_PointLightUBO);
    glNamedBufferStorage(m_PointLightUBO,
                          MaxPointLights * sizeof(GPUPointLight),
                          nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &m_SpotLightUBO);
    glNamedBufferStorage(m_SpotLightUBO,
                          MaxSpotLights * sizeof(GPUSpotLight),
                          nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void DeferredLightingSystem::LoadShaders() {
    m_GeometryShader = CreateRef<Shader>("assets/shaders/deferred/geometry.glsl");
    m_LightingShader = CreateRef<Shader>("assets/shaders/deferred/lighting.glsl");

    LOG_CORE_INFO("Deferred lighting shaders loaded");
}

} // namespace Engine
