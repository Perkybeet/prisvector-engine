#include "EditorIconRenderer.hpp"
#include "renderer/opengl/GLBuffer.hpp"
#include "resources/ResourceManager.hpp"
#include "ecs/Components/Renderable.hpp"
#include "ecs/Components/LightComponents.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

EditorIconRenderer::EditorIconRenderer() {
    auto& resources = ResourceManager::Instance();
    m_Shader = resources.LoadShader("billboard", "assets/shaders/editor/billboard.glsl");

    if (!m_Shader) {
        LOG_CORE_ERROR("EditorIconRenderer: Failed to load billboard shader");
        return;
    }

    // Create a simple quad for billboards
    float vertices[] = {
        // pos (x, y), texcoord (u, v)
        -0.5f, -0.5f, 0.0f, 0.0f,
        -0.5f,  0.5f, 0.0f, 1.0f,
         0.5f,  0.5f, 1.0f, 1.0f,
         0.5f, -0.5f, 1.0f, 0.0f
    };
    u32 indices[] = {0, 1, 2, 2, 3, 0};

    m_QuadVAO = CreateRef<VertexArray>();

    auto vbo = CreateRef<VertexBuffer>(vertices, sizeof(vertices));
    vbo->SetLayout({
        {ShaderDataType::Float2, "a_Position"},
        {ShaderDataType::Float2, "a_TexCoord"}
    });

    auto ibo = CreateRef<IndexBuffer>(indices, 6);

    m_QuadVAO->AddVertexBuffer(vbo);
    m_QuadVAO->SetIndexBuffer(ibo);

    LOG_CORE_INFO("EditorIconRenderer initialized");
}

EditorIconType EditorIconRenderer::DetermineIconType(entt::registry& registry, entt::entity entity) {
    if (registry.any_of<PointLightComponent>(entity)) return EditorIconType::PointLight;
    if (registry.any_of<SpotLightComponent>(entity)) return EditorIconType::SpotLight;
    if (registry.any_of<DirectionalLightComponent>(entity)) return EditorIconType::DirectionalLight;
    if (registry.any_of<AmbientLightComponent>(entity)) return EditorIconType::AmbientLight;

    // Check if entity has mesh - if not, it's an "empty" entity
    if (!registry.any_of<MeshComponent>(entity)) return EditorIconType::Empty;

    return EditorIconType::None;
}

glm::vec4 EditorIconRenderer::GetIconColor(EditorIconType type) const {
    switch (type) {
        case EditorIconType::PointLight:       return {1.0f, 0.9f, 0.3f, 1.0f};  // Yellow
        case EditorIconType::SpotLight:        return {1.0f, 0.6f, 0.2f, 1.0f};  // Orange
        case EditorIconType::DirectionalLight: return {1.0f, 1.0f, 0.5f, 1.0f};  // Light yellow
        case EditorIconType::AmbientLight:     return {0.5f, 0.5f, 1.0f, 1.0f};  // Light blue
        case EditorIconType::Empty:            return {0.6f, 0.6f, 0.6f, 1.0f};  // Gray
        default:                               return {1.0f, 1.0f, 1.0f, 1.0f};
    }
}

AABB EditorIconRenderer::GetIconBounds(const Transform& transform) const {
    glm::vec3 center = transform.Position;
    glm::vec3 halfExtent = glm::vec3(m_IconSize * 0.5f);
    return AABB(center - halfExtent, center + halfExtent);
}

void EditorIconRenderer::RenderIcon(const glm::vec3& position, EditorIconType type,
                                    const glm::mat4& viewProjection,
                                    const glm::vec3& cameraRight, const glm::vec3& cameraUp) {
    m_Shader->SetFloat3("u_WorldPosition", position);
    m_Shader->SetFloat("u_Size", m_IconSize);
    m_Shader->SetFloat3("u_CameraRight", cameraRight);
    m_Shader->SetFloat3("u_CameraUp", cameraUp);
    m_Shader->SetFloat4("u_Color", GetIconColor(type));
    m_Shader->SetInt("u_IconType", static_cast<i32>(type));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void EditorIconRenderer::Render(entt::registry& registry, const glm::mat4& view,
                                const glm::mat4& projection,
                                const glm::vec3& cameraRight, const glm::vec3& cameraUp) {
    if (!m_Visible || !m_Shader || !m_QuadVAO) return;

    // Save state
    GLboolean depthTest, blend, cullFace;
    GLint depthFunc;
    glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    glGetBooleanv(GL_BLEND, &blend);
    glGetBooleanv(GL_CULL_FACE, &cullFace);
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);

    // Setup rendering state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glm::mat4 viewProjection = projection * view;

    m_Shader->Bind();
    m_Shader->SetMat4("u_ViewProjection", viewProjection);
    m_QuadVAO->Bind();

    // Render icons for entities without meshes or with light components
    auto transformView = registry.view<Transform>();
    for (auto entity : transformView) {
        EditorIconType iconType = DetermineIconType(registry, entity);

        // Skip entities that don't need icons (have mesh and no special components)
        if (iconType == EditorIconType::None) continue;

        const auto& transform = transformView.get<Transform>(entity);
        RenderIcon(transform.Position, iconType, viewProjection, cameraRight, cameraUp);
    }

    // Restore state
    if (depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthFunc(depthFunc);
    if (blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (cullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
}

} // namespace Engine
