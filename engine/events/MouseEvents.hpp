#pragma once

#include "Event.hpp"
#include <sstream>

namespace Engine {

class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(f32 x, f32 y) : m_MouseX(x), m_MouseY(y) {}

    f32 GetX() const { return m_MouseX; }
    f32 GetY() const { return m_MouseY; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseMoved)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

private:
    f32 m_MouseX, m_MouseY;
};

class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(f32 xOffset, f32 yOffset)
        : m_XOffset(xOffset), m_YOffset(yOffset) {}

    f32 GetXOffset() const { return m_XOffset; }
    f32 GetYOffset() const { return m_YOffset; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "MouseScrolledEvent: " << m_XOffset << ", " << m_YOffset;
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseScrolled)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

private:
    f32 m_XOffset, m_YOffset;
};

class MouseButtonEvent : public Event {
public:
    i32 GetMouseButton() const { return m_Button; }

    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)

protected:
    MouseButtonEvent(i32 button) : m_Button(button) {}

    i32 m_Button;
};

class MouseButtonPressedEvent : public MouseButtonEvent {
public:
    MouseButtonPressedEvent(i32 button) : MouseButtonEvent(button) {}

    std::string ToString() const override {
        std::stringstream ss;
        ss << "MouseButtonPressedEvent: " << m_Button;
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    MouseButtonReleasedEvent(i32 button) : MouseButtonEvent(button) {}

    std::string ToString() const override {
        std::stringstream ss;
        ss << "MouseButtonReleasedEvent: " << m_Button;
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseButtonReleased)
};

} // namespace Engine
