#pragma once

#include "core/Types.hpp"
#include "renderer/opengl/GLShader.hpp"
#include "renderer/opengl/GLVertexArray.hpp"
#include "ecs/Components/Transform.hpp"
#include "math/AABB.hpp"
#include <glm/glm.hpp>
#include <entt/entt.hpp>

namespace Engine {

enum class EditorIconType : u8 {
    None = 0,
    Empty,
    PointLight,
    SpotLight,
    DirectionalLight,
    AmbientLight
};

class EditorIconRenderer {
public:
    EditorIconRenderer();
    ~EditorIconRenderer() = default;

    void Render(entt::registry& registry, const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraRight, const glm::vec3& cameraUp);

    // Get icon bounds for picking
    AABB GetIconBounds(const Transform& transform) const;

    // Determine icon type for an entity
    static EditorIconType DetermineIconType(entt::registry& registry, entt::entity entity);

    void SetIconSize(f32 size) { m_IconSize = size; }
    f32 GetIconSize() const { return m_IconSize; }

    void SetVisible(bool visible) { m_Visible = visible; }
    bool IsVisible() const { return m_Visible; }

private:
    glm::vec4 GetIconColor(EditorIconType type) const;
    void RenderIcon(const glm::vec3& position, EditorIconType type,
                    const glm::mat4& viewProjection,
                    const glm::vec3& cameraRight, const glm::vec3& cameraUp);

    Ref<Shader> m_Shader;
    Ref<VertexArray> m_QuadVAO;
    f32 m_IconSize = 1.5f;
    bool m_Visible = true;
};

} // namespace Engine
