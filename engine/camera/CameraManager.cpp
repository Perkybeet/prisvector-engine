#include "CameraManager.hpp"
#include "core/Logger.hpp"

namespace Engine {

void CameraManager::Register(const String& name, Scope<CameraController> controller) {
    if (m_Cameras.find(name) != m_Cameras.end()) {
        LOG_CORE_WARN("CameraManager: Overwriting camera '{}'", name);
    }

    m_Cameras[name] = std::move(controller);

    // If this is the first camera, make it active
    if (m_ActiveCamera == nullptr) {
        SetActive(name);
    }
}

void CameraManager::Unregister(const String& name) {
    auto it = m_Cameras.find(name);
    if (it == m_Cameras.end()) {
        LOG_CORE_WARN("CameraManager: Camera '{}' not found", name);
        return;
    }

    // If removing the active camera, clear it
    if (m_ActiveName == name) {
        m_ActiveCamera = nullptr;
        m_ActiveName.clear();
    }

    m_Cameras.erase(it);
}

void CameraManager::SetActive(const String& name) {
    auto it = m_Cameras.find(name);
    if (it == m_Cameras.end()) {
        LOG_CORE_WARN("CameraManager: Camera '{}' not found", name);
        return;
    }

    // Disable old camera
    if (m_ActiveCamera) {
        m_ActiveCamera->SetEnabled(false);
    }

    m_ActiveCamera = it->second.get();
    m_ActiveName = name;
    m_ActiveCamera->SetEnabled(true);
}

Camera* CameraManager::GetActiveCamera() {
    return m_ActiveCamera ? &m_ActiveCamera->GetCamera() : nullptr;
}

const Camera* CameraManager::GetActiveCamera() const {
    return m_ActiveCamera ? &m_ActiveCamera->GetCamera() : nullptr;
}

CameraController* CameraManager::Get(const String& name) {
    auto it = m_Cameras.find(name);
    return it != m_Cameras.end() ? it->second.get() : nullptr;
}

bool CameraManager::Has(const String& name) const {
    return m_Cameras.find(name) != m_Cameras.end();
}

void CameraManager::OnUpdate(f32 deltaTime) {
    if (m_ActiveCamera) {
        m_ActiveCamera->OnUpdate(deltaTime);
    }
}

void CameraManager::OnEvent(Event& event) {
    if (m_ActiveCamera) {
        m_ActiveCamera->OnEvent(event);
    }
}

Vector<String> CameraManager::GetCameraNames() const {
    Vector<String> names;
    names.reserve(m_Cameras.size());
    for (const auto& pair : m_Cameras) {
        names.push_back(pair.first);
    }
    return names;
}

} // namespace Engine
