module;

#include <windows.h>

export module Engine.Render.Window.Type;

import std;

export namespace Engine::Render::Window::Type
{
    // Signature of GetRawInputData from user32.
    using GetRawInputDataFunction = auto(WINAPI*)(HRAWINPUT, UINT, LPVOID, PUINT, UINT) -> UINT;

    // Window message captured by the WndProc hook.
    struct WindowMessage
    {
        void* Window{ nullptr };
        std::uint32_t Message{ 0 };
        std::uintptr_t WParam{ 0 };
        std::intptr_t LParam{ 0 };

        // Value returned to the window when a handler consumes the message.
        std::intptr_t Result{ 0 };
    };

    // Raw mouse movement and buttons captured by the GetRawInputData hook.
    struct RawMouse
    {
        std::int32_t DeltaX{ 0 };
        std::int32_t DeltaY{ 0 };

        std::uint16_t ButtonFlags{ 0 };
        std::uint16_t ButtonData{ 0 };
    };

    // Handler for window messages. Returns true to consume the message.
    using WindowMessageHandler = std::function<bool(WindowMessage&)>;

    // Handler for raw mouse movement. Returns true to swallow the movement.
    using RawMouseHandler = std::function<bool(RawMouse&)>;
}