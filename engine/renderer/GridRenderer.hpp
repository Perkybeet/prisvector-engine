#pragma once

#include "core/Types.hpp"
#include "renderer/opengl/GLShader.hpp"
#include "renderer/opengl/GLVertexArray.hpp"
#include <glm/glm.hpp>

namespace Engine {

class GridRenderer {
public:
    GridRenderer();
    ~GridRenderer() = default;

    void Render(const glm::mat4& viewProjection, const glm::vec3& cameraPos);

    void SetGridSize(f32 size) { m_GridSize = size; }
    f32 GetGridSize() const { return m_GridSize; }

    void SetFadeDistance(f32 distance) { m_FadeDistance = distance; }
    f32 GetFadeDistance() const { return m_FadeDistance; }

    void SetVisible(bool visible) { m_Visible = visible; }
    bool IsVisible() const { return m_Visible; }

private:
    Ref<Shader> m_Shader;
    Ref<VertexArray> m_GridVAO;
    f32 m_GridSize = 1.0f;
    f32 m_FadeDistance = 100.0f;
    bool m_Visible = true;
};

} // namespace Engine
