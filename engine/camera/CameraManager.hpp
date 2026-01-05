#pragma once

#include "core/Types.hpp"
#include "CameraController.hpp"
#include "events/Event.hpp"
#include <unordered_map>

namespace Engine {

class CameraManager {
public:
    CameraManager() = default;
    ~CameraManager() = default;

    // Register a camera controller with a name
    void Register(const String& name, Scope<CameraController> controller);

    // Remove a camera controller
    void Unregister(const String& name);

    // Set the active camera by name
    void SetActive(const String& name);

    // Get the active camera controller
    CameraController* GetActive() { return m_ActiveCamera; }
    const CameraController* GetActive() const { return m_ActiveCamera; }

    // Get the active camera
    Camera* GetActiveCamera();
    const Camera* GetActiveCamera() const;

    // Get a specific camera controller by name
    CameraController* Get(const String& name);

    // Check if a camera exists
    bool Has(const String& name) const;

    // Get the name of the active camera
    const String& GetActiveName() const { return m_ActiveName; }

    // Update the active camera
    void OnUpdate(f32 deltaTime);

    // Forward events to the active camera
    void OnEvent(Event& event);

    // Get all registered camera names
    Vector<String> GetCameraNames() const;

private:
    HashMap<String, Scope<CameraController>> m_Cameras;
    CameraController* m_ActiveCamera = nullptr;
    String m_ActiveName;
};

} // namespace Engine
