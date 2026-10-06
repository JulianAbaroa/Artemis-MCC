module;

#include <windows.h>

export module Platform.Input.System;

import Service.Logs.System;
import Platform.Input.Type;
import Platform.Input.State;
import std;

export namespace Platform::Input::System
{
    // Entry point for input. Dispatches window messages and raw mouse movement to handlers, and drives the AI input.
    class InputService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using InputAction = Platform::Input::Type::InputAction;
        using WindowMessage = Platform::Input::Type::WindowMessage;
        using RawMouse = Platform::Input::Type::RawMouse;
        using WindowMessageHandler = Platform::Input::Type::WindowMessageHandler;
        using RawMouseHandler = Platform::Input::Type::RawMouseHandler;

        using InputStore = Platform::Input::State::InputStore;
        using MouseDeltaStore = Platform::Input::State::MouseDeltaStore;

    public:
        InputService(LogsService& logsService, InputStore& inputStore,
            MouseDeltaStore& mouseDeltaStore) : m_LogsService(logsService),
            m_InputStore(inputStore), m_MouseDeltaStore(mouseDeltaStore) {}
        ~InputService() = default;

        // Registers a handler for window messages.
        auto OnWindowMessage(WindowMessageHandler handler) -> void;

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

        // Sets the game window that receives injected input.
        auto BindWindow(HWND window) -> void;

        // Enables or disables mouse injection by the AI.
        auto SetAIControlActive(bool active) -> void;

        // Queues mouse movement and posts a synthetic WM_INPUT so the game reads it.
        // return: False if AI control is off, no window is bound, or the post fails.
        auto InjectMouseDelta(std::int32_t deltaX, std::int32_t deltaY) -> bool;

        // Presses or releases a game action for the AI.
        auto SetActionRequested(InputAction action, bool requested) -> void;

        // Advances the action hold timers by one Artemis tick.
        // note: Called once per AI tick.
        auto AdvanceInputTick() -> void;

    private:
        LogsService& m_LogsService;
        InputStore& m_InputStore;
        MouseDeltaStore& m_MouseDeltaStore;

        std::atomic<HWND> m_Window{ nullptr };

        std::vector<WindowMessageHandler> m_OnWindowMessage{};
        std::vector<RawMouseHandler> m_OnRawMouse{};
        std::atomic<int> m_HandlerErrors{ 0 };

        // Logs a handler exception. Only the first 5 are logged.
        auto ReportHandlerError(const char* stage) -> void;
    };
}