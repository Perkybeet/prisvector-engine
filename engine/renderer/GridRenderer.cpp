#include "GridRenderer.hpp"
#include "renderer/opengl/GLBuffer.hpp"
#include "resources/ResourceManager.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>

namespace Engine {

GridRenderer::GridRenderer() {
    auto& resources = ResourceManager::Instance();
    m_Shader = resources.LoadShader("grid", "assets/shaders/editor/grid.glsl");

    if (!m_Shader) {
        LOG_CORE_ERROR("GridRenderer: Failed to load grid shader");
        return;
    }

    // Create a large XZ plane
    float gridExtent = 500.0f;
    float vertices[] = {
        // pos (x, y, z) - y is always 0 for XZ plane
        -gridExtent, 0.0f, -gridExtent,
        -gridExtent, 0.0f,  gridExtent,
         gridExtent, 0.0f,  gridExtent,
         gridExtent, 0.0f, -gridExtent
    };
    u32 indices[] = {0, 1, 2, 2, 3, 0};

    m_GridVAO = CreateRef<VertexArray>();

    auto vbo = CreateRef<VertexBuffer>(vertices, sizeof(vertices));
    vbo->SetLayout({
        {ShaderDataType::Float3, "a_Position"}
    });

    auto ibo = CreateRef<IndexBuffer>(indices, 6);

    m_GridVAO->AddVertexBuffer(vbo);
    m_GridVAO->SetIndexBuffer(ibo);

    LOG_CORE_INFO("GridRenderer initialized");
}

void GridRenderer::Render(const glm::mat4& viewProjection, const glm::vec3& cameraPos) {
    if (!m_Visible || !m_Shader || !m_GridVAO) return;

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

    m_Shader->Bind();
    m_Shader->SetMat4("u_ViewProjection", viewProjection);
    m_Shader->SetFloat("u_GridSize", m_GridSize);
    m_Shader->SetFloat("u_FadeDistance", m_FadeDistance);
    m_Shader->SetFloat3("u_CameraPos", cameraPos);

    m_GridVAO->Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    // Restore state
    if (depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthFunc(depthFunc);
    if (blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (cullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
}

} // namespace Engine
