#pragma once

#include "core/Types.hpp"
#include <glm/glm.hpp>

namespace Engine {

class Camera {
public:
    Camera() = default;
    virtual ~Camera() = default;

    const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }

    const glm::vec3& GetPosition() const { return m_Position; }

protected:
    glm::vec3 m_Position{0.0f, 0.0f, 0.0f};
    glm::mat4 m_ViewMatrix{1.0f};
    glm::mat4 m_ProjectionMatrix{1.0f};
};

} // namespace Engine
