#include "OrbitalCameraController.hpp"
#include "core/Input.hpp"
#include "math/Interpolation.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Engine {

OrbitalCameraController::OrbitalCameraController(f32 aspectRatio) {
    PerspectiveCameraSettings camSettings;
    camSettings.AspectRatio = aspectRatio;
    m_Camera = PerspectiveCamera(camSettings);

    UpdateCameraPosition(0.0f);
}

OrbitalCameraController::OrbitalCameraController(f32 aspectRatio, const OrbitalCameraSettings& settings)
    : m_FocusPoint(settings.FocusPoint)
    , m_TargetFocusPoint(settings.FocusPoint)
    , m_Radius(settings.Radius)
    , m_TargetRadius(settings.Radius)
    , m_MinRadius(settings.MinRadius)
    , m_MaxRadius(settings.MaxRadius)
    , m_Azimuth(settings.Azimuth)
    , m_Elevation(settings.Elevation)
    , m_TargetAzimuth(settings.Azimuth)
    , m_TargetElevation(settings.Elevation)
    , m_MinElevation(settings.MinElevation)
    , m_MaxElevation(settings.MaxElevation)
    , m_RotationSpeed(settings.RotationSpeed)
    , m_ZoomSpeed(settings.ZoomSpeed)
    , m_PanSpeed(settings.PanSpeed)
    , m_Smoothing(settings.Smoothing) {

    PerspectiveCameraSettings camSettings;
    camSettings.FOV = settings.FOV;
    camSettings.AspectRatio = aspectRatio;
    camSettings.NearPlane = settings.NearPlane;
    camSettings.FarPlane = settings.FarPlane;
    m_Camera = PerspectiveCamera(camSettings);

    UpdateCameraPosition(0.0f);
}

void OrbitalCameraController::OnUpdate(f32 deltaTime) {
    if (!m_Enabled) return;

    // Check mouse buttons for orbit/pan (respects ImGui capture)
    m_Orbiting = Input::IsGameMouseButtonPressed(Mouse::Left);
    m_Panning = Input::IsGameMouseButtonPressed(Mouse::Middle);

    // Smooth interpolation
    m_Azimuth = Math::ExpDecay(m_Azimuth, m_TargetAzimuth, m_Smoothing, deltaTime);
    m_Elevation = Math::ExpDecay(m_Elevation, m_TargetElevation, m_Smoothing, deltaTime);
    m_Radius = Math::ExpDecay(m_Radius, m_TargetRadius, m_Smoothing, deltaTime);
    m_FocusPoint = Math::ExpDecay(m_FocusPoint, m_TargetFocusPoint, m_Smoothing, deltaTime);

    UpdateCameraPosition(deltaTime);
}

void OrbitalCameraController::UpdateCameraPosition(f32 /*deltaTime*/) {
    // Convert spherical to cartesian coordinates
    f32 azimuthRad = glm::radians(m_Azimuth);
    f32 elevationRad = glm::radians(m_Elevation);

    glm::vec3 position;
    position.x = m_Radius * cos(elevationRad) * sin(azimuthRad);
    position.y = m_Radius * sin(elevationRad);
    position.z = m_Radius * cos(elevationRad) * cos(azimuthRad);

    position += m_FocusPoint;

    // Calculate camera orientation
    glm::vec3 front = glm::normalize(m_FocusPoint - position);
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    m_Camera.SetView(position, front, up);
}

void OrbitalCameraController::OnEvent(Event& event) {
    if (!m_Enabled) return;

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e) { return OnMouseMoved(e); });
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e) { return OnMouseScrolled(e); });
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
}

bool OrbitalCameraController::OnMouseMoved(MouseMovedEvent& event) {
    // Don't process mouse movement when ImGui wants the mouse
    if (Input::IsImGuiCapturingMouse()) return false;

    f32 xPos = event.GetX();
    f32 yPos = event.GetY();

    if (m_FirstMouse) {
        m_LastMousePosition = {xPos, yPos};
        m_FirstMouse = false;
        return false;
    }

    f32 xOffset = xPos - m_LastMousePosition.x;
    f32 yOffset = m_LastMousePosition.y - yPos;

    if (m_Orbiting) {
        m_TargetAzimuth += xOffset * m_RotationSpeed;
        m_TargetElevation += yOffset * m_RotationSpeed;
        m_TargetElevation = std::clamp(m_TargetElevation, m_MinElevation, m_MaxElevation);
    }

    if (m_Panning) {
        // Calculate pan vectors based on current orientation
        f32 azimuthRad = glm::radians(m_Azimuth);
        glm::vec3 right = glm::vec3(cos(azimuthRad), 0.0f, -sin(azimuthRad));
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        f32 panScale = m_Radius * m_PanSpeed;
        m_TargetFocusPoint -= right * xOffset * panScale;
        m_TargetFocusPoint += up * yOffset * panScale;
    }

    m_LastMousePosition = {xPos, yPos};
    return false;
}

bool OrbitalCameraController::OnMouseScrolled(MouseScrolledEvent& event) {
    // Don't process scroll when ImGui wants the mouse
    if (Input::IsImGuiCapturingMouse()) return false;

    m_TargetRadius -= event.GetYOffset() * m_ZoomSpeed * (m_Radius * 0.1f);
    m_TargetRadius = std::clamp(m_TargetRadius, m_MinRadius, m_MaxRadius);
    return false;
}

bool OrbitalCameraController::OnWindowResize(WindowResizeEvent& event) {
    if (event.GetWidth() > 0 && event.GetHeight() > 0) {
        f32 aspectRatio = static_cast<f32>(event.GetWidth()) / static_cast<f32>(event.GetHeight());
        m_Camera.SetAspectRatio(aspectRatio);
    }
    return false;
}

void OrbitalCameraController::SetRadius(f32 radius) {
    m_TargetRadius = std::clamp(radius, m_MinRadius, m_MaxRadius);
}

void OrbitalCameraController::SetElevation(f32 elevation) {
    m_TargetElevation = std::clamp(elevation, m_MinElevation, m_MaxElevation);
}

void OrbitalCameraController::Reset(const glm::vec3& focusPoint, f32 radius, f32 azimuth, f32 elevation) {
    m_FocusPoint = focusPoint;
    m_TargetFocusPoint = focusPoint;
    m_Radius = radius;
    m_TargetRadius = radius;
    m_Azimuth = azimuth;
    m_TargetAzimuth = azimuth;
    m_Elevation = elevation;
    m_TargetElevation = std::clamp(elevation, m_MinElevation, m_MaxElevation);

    UpdateCameraPosition(0.0f);
}

} // namespace Engine
