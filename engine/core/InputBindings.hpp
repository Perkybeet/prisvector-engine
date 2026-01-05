#pragma once

#include "Types.hpp"
#include "Input.hpp"
#include <unordered_map>
#include <vector>

namespace Engine {

struct InputBinding {
    i32 PrimaryKey = -1;
    i32 SecondaryKey = -1;
    i32 MouseButton = -1;

    InputBinding() = default;
    InputBinding(i32 primary) : PrimaryKey(primary) {}
    InputBinding(i32 primary, i32 secondary) : PrimaryKey(primary), SecondaryKey(secondary) {}
};

class InputBindings {
public:
    InputBindings() = default;

    // Bind a key to an action
    void Bind(const String& action, i32 keycode);
    void Bind(const String& action, i32 primary, i32 secondary);
    void BindMouse(const String& action, i32 mouseButton);

    // Unbind an action
    void Unbind(const String& action);

    // Get the binding for an action
    const InputBinding* GetBinding(const String& action) const;

    // Query action states
    bool IsActionPressed(const String& action) const;
    bool IsActionJustPressed(const String& action) const;
    bool IsActionJustReleased(const String& action) const;

    // Get action strength (for analog-like behavior, 0.0 or 1.0 for digital)
    f32 GetActionStrength(const String& action) const;

    // Get axis value (positive - negative actions)
    f32 GetAxis(const String& positiveAction, const String& negativeAction) const;

    // Get all registered action names
    Vector<String> GetActionNames() const;

    // Clear all bindings
    void Clear();

private:
    HashMap<String, InputBinding> m_Bindings;
};

// Predefined common action bindings
namespace Actions {
    // Movement
    constexpr const char* MoveForward = "move_forward";
    constexpr const char* MoveBackward = "move_backward";
    constexpr const char* MoveLeft = "move_left";
    constexpr const char* MoveRight = "move_right";
    constexpr const char* MoveUp = "move_up";
    constexpr const char* MoveDown = "move_down";
    constexpr const char* Sprint = "sprint";

    // Camera
    constexpr const char* LookUp = "look_up";
    constexpr const char* LookDown = "look_down";
    constexpr const char* LookLeft = "look_left";
    constexpr const char* LookRight = "look_right";

    // Actions
    constexpr const char* Jump = "jump";
    constexpr const char* Crouch = "crouch";
    constexpr const char* Interact = "interact";
    constexpr const char* Attack = "attack";
    constexpr const char* SecondaryAttack = "secondary_attack";

    // UI
    constexpr const char* Pause = "pause";
    constexpr const char* Confirm = "confirm";
    constexpr const char* Cancel = "cancel";
}

} // namespace Engine
