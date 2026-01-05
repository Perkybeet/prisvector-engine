#include "DebugRenderer.hpp"
#include "renderer/shadows/CascadedShadowMap.hpp"
#include "renderer/pipeline/GBuffer.hpp"
#include "resources/ResourceManager.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>

namespace Engine {

DebugRenderer::DebugRenderer() = default;

DebugRenderer::~DebugRenderer() {
    Shutdown();
}

void DebugRenderer::Initialize() {
    if (m_Initialized) return;

    auto& resources = ResourceManager::Instance();
    m_DebugShader = resources.LoadShader("debug_texture", "assets/shaders/debug/debug_texture.glsl");

    if (!m_DebugShader) {
        LOG_CORE_ERROR("DebugRenderer: Failed to load debug_texture shader");
        return;
    }

    CreateQuad();
    m_Initialized = true;

    LOG_CORE_INFO("DebugRenderer initialized");
}

void DebugRenderer::Shutdown() {
    if (!m_Initialized) return;

    m_QuadVAO.reset();
    m_DebugShader.reset();
    m_Initialized = false;
}

void DebugRenderer::CreateQuad() {
    float vertices[] = {
        // pos           // uv
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };
    u32 indices[] = {0, 1, 2, 2, 3, 0};

    m_QuadVAO = CreateRef<VertexArray>();

    auto vbo = CreateRef<VertexBuffer>(vertices, sizeof(vertices));
    vbo->SetLayout({
        {ShaderDataType::Float3, "a_Position"},
        {ShaderDataType::Float2, "a_TexCoords"}
    });

    auto ibo = CreateRef<IndexBuffer>(indices, 6);

    m_QuadVAO->AddVertexBuffer(vbo);
    m_QuadVAO->SetIndexBuffer(ibo);
}

void DebugRenderer::Render(u32 windowWidth, u32 windowHeight) {
    if (!m_Initialized || m_ActiveView == DebugView::None) return;

    // Save OpenGL state
    GLboolean depthTest, blend;
    glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    glGetBooleanv(GL_BLEND, &blend);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    switch (m_ActiveView) {
        case DebugView::CSMCascades:
            RenderCSMCascades(windowWidth, windowHeight);
            break;
        case DebugView::GBufferAlbedo:
        case DebugView::GBufferNormals:
        case DebugView::GBufferDepth:
        case DebugView::GBufferMetalRough:
            RenderGBufferView(windowWidth, windowHeight);
            break;
        default:
            break;
    }

    // Restore state
    if (depthTest) glEnable(GL_DEPTH_TEST);
    if (blend) glEnable(GL_BLEND);
}

void DebugRenderer::RenderQuad(f32 x, f32 y, f32 w, f32 h, u32 textureId, i32 layer, i32 mode) {
    m_DebugShader->Bind();
    m_DebugShader->SetFloat4("u_Viewport", glm::vec4(x, y, w, h));
    m_DebugShader->SetInt("u_Mode", mode);
    m_DebugShader->SetInt("u_Layer", layer);
    m_DebugShader->SetFloat("u_NearPlane", 0.1f);
    m_DebugShader->SetFloat("u_FarPlane", 1000.0f);

    if (mode == 0) {
        // CSM depth array
        m_DebugShader->SetInt("u_DepthArray", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureId);
    } else {
        // 2D texture
        m_DebugShader->SetInt("u_Texture2D", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    m_QuadVAO->Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void DebugRenderer::RenderCSMCascades(u32 windowWidth, u32 windowHeight) {
    if (!m_CSM) {
        LOG_CORE_WARN("DebugRenderer: No CSM set for debug view");
        return;
    }

    f32 size = m_ViewportScale;
    f32 padding = 0.01f;

    // Render 4 cascades in a column on the right side of the screen
    for (u32 i = 0; i < CSM_CASCADE_COUNT; i++) {
        f32 x = 1.0f - size - padding;
        f32 y = padding + (size + padding) * static_cast<f32>(i);

        RenderQuad(x, y, size, size, m_CSM->GetTextureID(), static_cast<i32>(i), 0);
    }
}

void DebugRenderer::RenderGBufferView(u32 windowWidth, u32 windowHeight) {
    if (!m_GBuffer) {
        LOG_CORE_WARN("DebugRenderer: No GBuffer set for debug view");
        return;
    }

    f32 size = m_ViewportScale * 2.0f;  // Larger single view
    f32 x = 1.0f - size - 0.01f;
    f32 y = 0.01f;

    u32 textureId = 0;
    i32 mode = 1;  // Regular color texture

    switch (m_ActiveView) {
        case DebugView::GBufferAlbedo:
            textureId = m_GBuffer->GetAlbedoTextureID();
            break;
        case DebugView::GBufferNormals:
            textureId = m_GBuffer->GetNormalTextureID();
            mode = 3;  // Normal visualization mode
            break;
        case DebugView::GBufferDepth:
            textureId = m_GBuffer->GetDepthTextureID();
            mode = 2;  // Depth visualization
            break;
        case DebugView::GBufferMetalRough:
            textureId = m_GBuffer->GetPositionTextureID();  // Position has depth in W
            break;
        default:
            return;
    }

    RenderQuad(x, y, size, size, textureId, 0, mode);
}

void DebugRenderer::CycleView() {
    u32 current = static_cast<u32>(m_ActiveView);
    current = (current + 1) % static_cast<u32>(DebugView::Count);
    m_ActiveView = static_cast<DebugView>(current);

    const char* viewNames[] = {"None", "CSM Cascades", "GBuffer Albedo",
                                "GBuffer Normals", "GBuffer Depth", "GBuffer Metal/Rough"};
    LOG_CORE_INFO("Debug View: {}", viewNames[current]);
}

void DebugRenderer::ToggleView(DebugView view) {
    if (m_ActiveView == view) {
        m_ActiveView = DebugView::None;
    } else {
        m_ActiveView = view;
    }
}

} // namespace Engine
