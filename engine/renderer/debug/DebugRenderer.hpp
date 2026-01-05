#pragma once

#include "core/Types.hpp"
#include "renderer/opengl/GLShader.hpp"
#include "renderer/opengl/GLVertexArray.hpp"
#include "renderer/shadows/ShadowTypes.hpp"

namespace Engine {

// Forward declarations
class CascadedShadowMap;
class GBuffer;

enum class DebugView : u32 {
    None = 0,
    CSMCascades,        // Show all 4 CSM cascade depth maps
    GBufferAlbedo,      // Albedo/color from GBuffer
    GBufferNormals,     // World-space normals
    GBufferDepth,       // Depth buffer
    GBufferMetalRough,  // Metallic/Roughness packed texture
    Count
};

class DebugRenderer {
public:
    DebugRenderer();
    ~DebugRenderer();

    void Initialize();
    void Shutdown();

    // Set sources for debug visualization
    void SetCSM(const CascadedShadowMap* csm) { m_CSM = csm; }
    void SetGBuffer(const GBuffer* gbuffer) { m_GBuffer = gbuffer; }

    // Render debug overlay
    void Render(u32 windowWidth, u32 windowHeight);

    // Control active view
    void SetActiveView(DebugView view) { m_ActiveView = view; }
    DebugView GetActiveView() const { return m_ActiveView; }
    void CycleView();
    void ToggleView(DebugView view);

    bool IsEnabled() const { return m_ActiveView != DebugView::None; }

    // Size of each debug viewport (as fraction of screen width)
    void SetViewportScale(f32 scale) { m_ViewportScale = scale; }

private:
    void CreateQuad();
    void RenderQuad(f32 x, f32 y, f32 w, f32 h, u32 textureId, i32 layer, i32 mode);
    void RenderCSMCascades(u32 windowWidth, u32 windowHeight);
    void RenderGBufferView(u32 windowWidth, u32 windowHeight);

private:
    Ref<Shader> m_DebugShader;
    Ref<VertexArray> m_QuadVAO;

    const CascadedShadowMap* m_CSM = nullptr;
    const GBuffer* m_GBuffer = nullptr;

    DebugView m_ActiveView = DebugView::None;
    f32 m_ViewportScale = 0.2f;  // Each viewport is 20% of screen width

    bool m_Initialized = false;
};

} // namespace Engine
