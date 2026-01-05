#include "ThirdPersonCameraController.hpp"
#include "core/Input.hpp"
#include "math/Interpolation.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Engine {

ThirdPersonCameraController::ThirdPersonCameraController(f32 aspectRatio) {
    PerspectiveCameraSettings camSettings;
    camSettings.AspectRatio = aspectRatio;
    m_Camera = PerspectiveCamera(camSettings);

    UpdateCameraPosition(0.0f);
}

ThirdPersonCameraController::ThirdPersonCameraController(f32 aspectRatio, const ThirdPersonCameraSettings& settings)
    : m_Distance(settings.Distance)
    , m_TargetDistance(settings.Distance)
    , m_MinDistance(settings.MinDistance)
    , m_MaxDistance(settings.MaxDistance)
    , m_HeightOffset(settings.HeightOffset)
    , m_RotationSpeed(settings.RotationSpeed)
    , m_ZoomSpeed(settings.ZoomSpeed)
    , m_Smoothing(settings.Smoothing)
    , m_Yaw(settings.InitialYaw)
    , m_Pitch(settings.InitialPitch)
    , m_TargetYaw(settings.InitialYaw)
    , m_TargetPitch(settings.InitialPitch)
    , m_MinPitch(settings.MinPitch)
    , m_MaxPitch(settings.MaxPitch) {

    PerspectiveCameraSettings camSettings;
    camSettings.FOV = settings.FOV;
    camSettings.AspectRatio = aspectRatio;
    camSettings.NearPlane = settings.NearPlane;
    camSettings.FarPlane = settings.FarPlane;
    m_Camera = PerspectiveCamera(camSettings);

    UpdateCameraPosition(0.0f);
}

void ThirdPersonCameraController::OnUpdate(f32 deltaTime) {
    if (!m_Enabled) return;

    // Check if right mouse button is held for rotation (respects ImGui capture)
    m_Rotating = Input::IsGameMouseButtonPressed(Mouse::Right);

    // Smooth interpolation
    m_Yaw = Math::ExpDecay(m_Yaw, m_TargetYaw, m_Smoothing, deltaTime);
    m_Pitch = Math::ExpDecay(m_Pitch, m_TargetPitch, m_Smoothing, deltaTime);
    m_Distance = Math::ExpDecay(m_Distance, m_TargetDistance, m_Smoothing, deltaTime);

    UpdateCameraPosition(deltaTime);
}

void ThirdPersonCameraController::UpdateCameraPosition(f32 deltaTime) {
    // Calculate camera position based on spherical coordinates
    f32 yawRad = glm::radians(m_Yaw);
    f32 pitchRad = glm::radians(m_Pitch);

    glm::vec3 offset;
    offset.x = m_Distance * cos(pitchRad) * sin(yawRad);
    offset.y = m_Distance * sin(pitchRad) + m_HeightOffset;
    offset.z = m_Distance * cos(pitchRad) * cos(yawRad);

    glm::vec3 targetPos = m_TargetPosition + glm::vec3(0.0f, m_HeightOffset, 0.0f);
    glm::vec3 desiredPosition = m_TargetPosition + offset;

    // Smooth camera position
    if (deltaTime > 0.0f) {
        m_CurrentPosition = Math::ExpDecay(m_CurrentPosition, desiredPosition, m_Smoothing, deltaTime);
    } else {
        m_CurrentPosition = desiredPosition;
    }

    // Look at target
    glm::vec3 front = glm::normalize(targetPos - m_CurrentPosition);
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    m_Camera.SetView(m_CurrentPosition, front, up);
}

void ThirdPersonCameraController::OnEvent(Event& event) {
    if (!m_Enabled) return;

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e) { return OnMouseMoved(e); });
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e) { return OnMouseScrolled(e); });
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
}

bool ThirdPersonCameraController::OnMouseMoved(MouseMovedEvent& event) {
    // Don't process mouse movement when ImGui wants the mouse
    if (Input::IsImGuiCapturingMouse()) return false;

    f32 xPos = event.GetX();
    f32 yPos = event.GetY();

    if (m_FirstMouse) {
        m_LastMousePosition = {xPos, yPos};
        m_FirstMouse = false;
        return false;
    }

    if (m_Rotating) {
        f32 xOffset = (xPos - m_LastMousePosition.x) * m_RotationSpeed;
        f32 yOffset = (m_LastMousePosition.y - yPos) * m_RotationSpeed;

        m_TargetYaw += xOffset;
        m_TargetPitch += yOffset;
        m_TargetPitch = std::clamp(m_TargetPitch, m_MinPitch, m_MaxPitch);
    }

    m_LastMousePosition = {xPos, yPos};
    return false;
}

bool ThirdPersonCameraController::OnMouseScrolled(MouseScrolledEvent& event) {
    // Don't process scroll when ImGui wants the mouse
    if (Input::IsImGuiCapturingMouse()) return false;

    m_TargetDistance -= event.GetYOffset() * m_ZoomSpeed;
    m_TargetDistance = std::clamp(m_TargetDistance, m_MinDistance, m_MaxDistance);
    return false;
}

bool ThirdPersonCameraController::OnWindowResize(WindowResizeEvent& event) {
    if (event.GetWidth() > 0 && event.GetHeight() > 0) {
        f32 aspectRatio = static_cast<f32>(event.GetWidth()) / static_cast<f32>(event.GetHeight());
        m_Camera.SetAspectRatio(aspectRatio);
    }
    return false;
}

void ThirdPersonCameraController::SetDistance(f32 distance) {
    m_TargetDistance = std::clamp(distance, m_MinDistance, m_MaxDistance);
    m_Distance = m_TargetDistance;
}

} // namespace Engine
