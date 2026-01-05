#include "PerspectiveCamera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

PerspectiveCamera::PerspectiveCamera(const PerspectiveCameraSettings& settings)
    : m_FOV(settings.FOV)
    , m_AspectRatio(settings.AspectRatio)
    , m_NearPlane(settings.NearPlane)
    , m_FarPlane(settings.FarPlane) {
    RecalculateProjection();
}

void PerspectiveCamera::SetPerspective(f32 fov, f32 aspectRatio, f32 nearPlane, f32 farPlane) {
    m_FOV = fov;
    m_AspectRatio = aspectRatio;
    m_NearPlane = nearPlane;
    m_FarPlane = farPlane;
    RecalculateProjection();
}

void PerspectiveCamera::SetAspectRatio(f32 aspectRatio) {
    m_AspectRatio = aspectRatio;
    RecalculateProjection();
}

void PerspectiveCamera::SetFOV(f32 fov) {
    m_FOV = fov;
    RecalculateProjection();
}

void PerspectiveCamera::SetView(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up) {
    m_Position = position;
    m_ViewMatrix = glm::lookAt(position, position + front, up);
}

void PerspectiveCamera::RecalculateProjection() {
    m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearPlane, m_FarPlane);
}

} // namespace Engine
