#pragma once

#include "CameraController.hpp"
#include "PerspectiveCamera.hpp"
#include "events/MouseEvents.hpp"
#include "events/WindowEvents.hpp"
#include <glm/glm.hpp>

namespace Engine {

struct OrbitalCameraSettings {
    glm::vec3 FocusPoint{0.0f};
    f32 Radius = 5.0f;
    f32 MinRadius = 1.0f;
    f32 MaxRadius = 50.0f;
    f32 Azimuth = 0.0f;      // Horizontal angle (degrees)
    f32 Elevation = 30.0f;   // Vertical angle (degrees)
    f32 MinElevation = -89.0f;
    f32 MaxElevation = 89.0f;
    f32 RotationSpeed = 0.5f;
    f32 ZoomSpeed = 0.5f;
    f32 PanSpeed = 0.01f;
    f32 Smoothing = 12.0f;
    f32 FOV = 45.0f;
    f32 NearPlane = 0.1f;
    f32 FarPlane = 1000.0f;
};

class OrbitalCameraController : public CameraController {
public:
    explicit OrbitalCameraController(f32 aspectRatio);
    OrbitalCameraController(f32 aspectRatio, const OrbitalCameraSettings& settings);

    void OnUpdate(f32 deltaTime) override;
    void OnEvent(Event& event) override;

    Camera& GetCamera() override { return m_Camera; }
    const Camera& GetCamera() const override { return m_Camera; }

    // Focus point control
    void SetFocusPoint(const glm::vec3& point) { m_TargetFocusPoint = point; }
    const glm::vec3& GetFocusPoint() const { return m_FocusPoint; }

    // Orbital parameters
    void SetRadius(f32 radius);
    f32 GetRadius() const { return m_Radius; }

    void SetAzimuth(f32 azimuth) { m_TargetAzimuth = azimuth; }
    void SetElevation(f32 elevation);

    f32 GetAzimuth() const { return m_Azimuth; }
    f32 GetElevation() const { return m_Elevation; }

    void SetSmoothing(f32 smoothing) { m_Smoothing = smoothing; }

    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

    // Reset to initial view
    void Reset(const glm::vec3& focusPoint, f32 radius, f32 azimuth, f32 elevation);

private:
    void UpdateCameraPosition(f32 deltaTime);

    bool OnMouseMoved(MouseMovedEvent& event);
    bool OnMouseScrolled(MouseScrolledEvent& event);
    bool OnWindowResize(WindowResizeEvent& event);

private:
    PerspectiveCamera m_Camera;

    glm::vec3 m_FocusPoint{0.0f};
    glm::vec3 m_TargetFocusPoint{0.0f};

    f32 m_Radius = 5.0f;
    f32 m_TargetRadius = 5.0f;
    f32 m_MinRadius = 1.0f;
    f32 m_MaxRadius = 50.0f;

    f32 m_Azimuth = 0.0f;
    f32 m_Elevation = 30.0f;
    f32 m_TargetAzimuth = 0.0f;
    f32 m_TargetElevation = 30.0f;
    f32 m_MinElevation = -89.0f;
    f32 m_MaxElevation = 89.0f;

    f32 m_RotationSpeed = 0.5f;
    f32 m_ZoomSpeed = 0.5f;
    f32 m_PanSpeed = 0.01f;
    f32 m_Smoothing = 12.0f;

    glm::vec2 m_LastMousePosition{0.0f};
    bool m_FirstMouse = true;
    bool m_Enabled = true;
    bool m_Orbiting = false;
    bool m_Panning = false;
};

} // namespace Engine
