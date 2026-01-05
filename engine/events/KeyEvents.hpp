#pragma once

#include "Event.hpp"
#include <sstream>

namespace Engine {

class KeyEvent : public Event {
public:
    i32 GetKeyCode() const { return m_KeyCode; }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

protected:
    KeyEvent(i32 keycode) : m_KeyCode(keycode) {}

    i32 m_KeyCode;
};

class KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(i32 keycode, bool isRepeat = false)
        : KeyEvent(keycode), m_IsRepeat(isRepeat) {}

    bool IsRepeat() const { return m_IsRepeat; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << m_KeyCode << " (repeat = " << m_IsRepeat << ")";
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyPressed)

private:
    bool m_IsRepeat;
};

class KeyReleasedEvent : public KeyEvent {
public:
    KeyReleasedEvent(i32 keycode) : KeyEvent(keycode) {}

    std::string ToString() const override {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << m_KeyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent {
public:
    KeyTypedEvent(i32 keycode) : KeyEvent(keycode) {}

    std::string ToString() const override {
        std::stringstream ss;
        ss << "KeyTypedEvent: " << m_KeyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyTyped)
};

} // namespace Engine
