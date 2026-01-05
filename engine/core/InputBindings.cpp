#include "InputBindings.hpp"

namespace Engine {

void InputBindings::Bind(const String& action, i32 keycode) {
    m_Bindings[action] = InputBinding(keycode);
}

void InputBindings::Bind(const String& action, i32 primary, i32 secondary) {
    m_Bindings[action] = InputBinding(primary, secondary);
}

void InputBindings::BindMouse(const String& action, i32 mouseButton) {
    auto it = m_Bindings.find(action);
    if (it != m_Bindings.end()) {
        it->second.MouseButton = mouseButton;
    } else {
        InputBinding binding;
        binding.MouseButton = mouseButton;
        m_Bindings[action] = binding;
    }
}

void InputBindings::Unbind(const String& action) {
    m_Bindings.erase(action);
}

const InputBinding* InputBindings::GetBinding(const String& action) const {
    auto it = m_Bindings.find(action);
    return it != m_Bindings.end() ? &it->second : nullptr;
}

bool InputBindings::IsActionPressed(const String& action) const {
    const InputBinding* binding = GetBinding(action);
    if (!binding) return false;

    if (binding->PrimaryKey >= 0 && Input::IsKeyPressed(binding->PrimaryKey))
        return true;
    if (binding->SecondaryKey >= 0 && Input::IsKeyPressed(binding->SecondaryKey))
        return true;
    if (binding->MouseButton >= 0 && Input::IsMouseButtonPressed(binding->MouseButton))
        return true;

    return false;
}

bool InputBindings::IsActionJustPressed(const String& action) const {
    const InputBinding* binding = GetBinding(action);
    if (!binding) return false;

    if (binding->PrimaryKey >= 0 && Input::IsKeyJustPressed(binding->PrimaryKey))
        return true;
    if (binding->SecondaryKey >= 0 && Input::IsKeyJustPressed(binding->SecondaryKey))
        return true;
    if (binding->MouseButton >= 0 && Input::IsMouseButtonJustPressed(binding->MouseButton))
        return true;

    return false;
}

bool InputBindings::IsActionJustReleased(const String& action) const {
    const InputBinding* binding = GetBinding(action);
    if (!binding) return false;

    // For release, check if any bound input was just released
    // AND no other bound input is still pressed
    bool anyReleased = false;
    bool anyStillPressed = false;

    if (binding->PrimaryKey >= 0) {
        if (Input::IsKeyJustReleased(binding->PrimaryKey)) anyReleased = true;
        if (Input::IsKeyPressed(binding->PrimaryKey)) anyStillPressed = true;
    }
    if (binding->SecondaryKey >= 0) {
        if (Input::IsKeyJustReleased(binding->SecondaryKey)) anyReleased = true;
        if (Input::IsKeyPressed(binding->SecondaryKey)) anyStillPressed = true;
    }
    if (binding->MouseButton >= 0) {
        if (Input::IsMouseButtonJustReleased(binding->MouseButton)) anyReleased = true;
        if (Input::IsMouseButtonPressed(binding->MouseButton)) anyStillPressed = true;
    }

    return anyReleased && !anyStillPressed;
}

f32 InputBindings::GetActionStrength(const String& action) const {
    return IsActionPressed(action) ? 1.0f : 0.0f;
}

f32 InputBindings::GetAxis(const String& positiveAction, const String& negativeAction) const {
    f32 positive = GetActionStrength(positiveAction);
    f32 negative = GetActionStrength(negativeAction);
    return positive - negative;
}

Vector<String> InputBindings::GetActionNames() const {
    Vector<String> names;
    names.reserve(m_Bindings.size());
    for (const auto& pair : m_Bindings) {
        names.push_back(pair.first);
    }
    return names;
}

void InputBindings::Clear() {
    m_Bindings.clear();
}

} // namespace Engine
