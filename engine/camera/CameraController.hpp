#pragma once

#include "core/Types.hpp"
#include "events/Event.hpp"

namespace Engine {

class Camera;

class CameraController {
public:
    virtual ~CameraController() = default;

    virtual void OnUpdate(f32 deltaTime) = 0;
    virtual void OnEvent(Event& event) = 0;

    virtual Camera& GetCamera() = 0;
    virtual const Camera& GetCamera() const = 0;

    virtual void SetEnabled(bool enabled) = 0;
    virtual bool IsEnabled() const = 0;
};

} // namespace Engine
