#pragma once

#include "Types.hpp"
#include <glm/glm.hpp>

struct GLFWwindow;

namespace Engine {

class Input {
public:
    static void Init(GLFWwindow* window);

    // Call at the start of each frame to update input state
    static void Update();

    // Key state queries
    static bool IsKeyPressed(i32 keycode);
    static bool IsKeyJustPressed(i32 keycode);
    static bool IsKeyJustReleased(i32 keycode);

    // Mouse button state queries
    static bool IsMouseButtonPressed(i32 button);
    static bool IsMouseButtonJustPressed(i32 button);
    static bool IsMouseButtonJustReleased(i32 button);

    // Mouse position
    static glm::vec2 GetMousePosition();
    static f32 GetMouseX();
    static f32 GetMouseY();

    // Mouse delta (change since last frame)
    static glm::vec2 GetMouseDelta();
    static f32 GetMouseDeltaX();
    static f32 GetMouseDeltaY();

    // Scroll delta
    static glm::vec2 GetScrollDelta();

    static void SetCursorMode(bool enabled);

    // Internal: called by window to set scroll values
    static void SetScrollOffset(f32 xOffset, f32 yOffset);

    // ImGui awareness - check if ImGui wants to capture input
    static bool IsImGuiCapturingMouse();
    static bool IsImGuiCapturingKeyboard();

    // Game-aware input - these respect ImGui capture state
    // Use these in game logic (cameras, player controls, etc.)
    static bool IsGameKeyPressed(i32 keycode);
    static bool IsGameKeyJustPressed(i32 keycode);
    static bool IsGameKeyJustReleased(i32 keycode);
    static bool IsGameMouseButtonPressed(i32 button);
    static bool IsGameMouseButtonJustPressed(i32 button);
    static bool IsGameMouseButtonJustReleased(i32 button);
    static glm::vec2 GetGameMouseDelta();
    static glm::vec2 GetGameScrollDelta();

private:
    static constexpr i32 MAX_KEYS = 512;
    static constexpr i32 MAX_MOUSE_BUTTONS = 8;

    static GLFWwindow* s_Window;

    static bool s_CurrentKeys[MAX_KEYS];
    static bool s_PreviousKeys[MAX_KEYS];

    static bool s_CurrentMouseButtons[MAX_MOUSE_BUTTONS];
    static bool s_PreviousMouseButtons[MAX_MOUSE_BUTTONS];

    static glm::vec2 s_MousePosition;
    static glm::vec2 s_LastMousePosition;
    static glm::vec2 s_MouseDelta;

    static glm::vec2 s_ScrollDelta;
    static glm::vec2 s_ScrollAccumulator;

    static bool s_FirstMouse;
};

// Key codes (matching GLFW)
namespace Key {
    enum : i32 {
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,

        D0 = 48, D1 = 49, D2 = 50, D3 = 51, D4 = 52,
        D5 = 53, D6 = 54, D7 = 55, D8 = 56, D9 = 57,

        Semicolon = 59,
        Equal = 61,

        A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72,
        I = 73, J = 74, K = 75, L = 76, M = 77, N = 78, O = 79, P = 80,
        Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
        Y = 89, Z = 90,

        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        GraveAccent = 96,

        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,

        F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295,
        F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,

        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347,
        Menu = 348
    };
}

namespace Mouse {
    enum : i32 {
        Button1 = 0,
        Button2 = 1,
        Button3 = 2,
        Button4 = 3,
        Button5 = 4,
        Button6 = 5,
        Button7 = 6,
        Button8 = 7,
        Left = Button1,
        Right = Button2,
        Middle = Button3
    };
}

} // namespace Engine
