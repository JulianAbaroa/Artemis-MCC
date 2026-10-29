export module Engine.Render.Window.System:Input;

import Common.Log.System;
import Engine.Render.Window.Type;
import std;

export namespace Engine::Render::Window::System
{
    // Entry point for input. Dispatches window messages and raw mouse movement to handlers.
    class InputService
    {
    private:
        using Log = Common::Log::System::Log;

        using WindowMessage = Engine::Render::Window::Type::WindowMessage;
        using RawMouse = Engine::Render::Window::Type::RawMouse;
        using WindowMessageHandler = Engine::Render::Window::Type::WindowMessageHandler;
        using RawMouseHandler = Engine::Render::Window::Type::RawMouseHandler;

    public:
        explicit InputService(Log& log) : m_Log(log) {}
        ~InputService() = default;

        // Registers a handler for window messages.
        // param runFirst: True to call it before the handlers registered earlier, so they cannot swallow its messages.
        auto OnWindowMessage(WindowMessageHandler handler, bool runFirst = false) -> void;

        // Registers a handler for raw mouse movement.
        auto OnRawMouse(RawMouseHandler handler) -> void;

        // Calls the handlers in registration order until one consumes the message.
        // return: True if a handler consumed it.
        // note: A handler that throws is skipped and reported.
        auto RaiseWindowMessage(WindowMessage& message) -> bool;

        // Calls the handlers in registration order until one swallows the movement.
        // return: True if a handler swallowed it.
        // note: A handler that throws is skipped and reported.
        auto RaiseRawMouse(RawMouse& mouse) -> bool;

    private:
        Log& m_Log;

        std::vector<WindowMessageHandler> m_OnWindowMessage{};
        std::vector<RawMouseHandler> m_OnRawMouse{};
        std::atomic<int> m_HandlerErrors{ 0 };

        // Logs a handler exception. Only the first 5 are logged.
        auto ReportHandlerError(const char* stage) -> void;
    };
}