#pragma once

#include "Camera.hpp"

namespace Engine {

struct PerspectiveCameraSettings {
    f32 FOV = 45.0f;
    f32 AspectRatio = 16.0f / 9.0f;
    f32 NearPlane = 0.1f;
    f32 FarPlane = 1000.0f;
};

class PerspectiveCamera : public Camera {
public:
    PerspectiveCamera() = default;
    explicit PerspectiveCamera(const PerspectiveCameraSettings& settings);

    void SetPerspective(f32 fov, f32 aspectRatio, f32 nearPlane, f32 farPlane);
    void SetAspectRatio(f32 aspectRatio);
    void SetFOV(f32 fov);
    void SetView(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up);

    f32 GetFOV() const { return m_FOV; }
    f32 GetAspectRatio() const { return m_AspectRatio; }
    f32 GetNearPlane() const { return m_NearPlane; }
    f32 GetFarPlane() const { return m_FarPlane; }

private:
    void RecalculateProjection();

    f32 m_FOV = 45.0f;
    f32 m_AspectRatio = 16.0f / 9.0f;
    f32 m_NearPlane = 0.1f;
    f32 m_FarPlane = 1000.0f;
};

} // namespace Engine
