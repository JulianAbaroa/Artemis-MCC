module;

#include <windows.h>

export module Platform.Input.System;

import Service.Logs.System;
import Platform.Input.Type;
import Platform.Input.State;
import std;

export namespace Platform::Input::System
{
    class InputService
    {
    private:
        using Action = Platform::Input::Type::Action;
        using WindowMessage = Platform::Input::Type::WindowMessage;
        using RawMouse = Platform::Input::Type::RawMouse;

        using LogsService = Service::Logs::System::LogsService;
        using InputStore = Platform::Input::State::InputStore;
        using MouseDeltaStore = Platform::Input::State::MouseDeltaStore;

    public:
        using WindowMessageHandler = std::function<bool(WindowMessage&)>;
        using RawMouseHandler = std::function<bool(RawMouse&)>;

        InputService(LogsService& logsService, InputStore& inputStore,
            MouseDeltaStore& mouseDeltaStore) : m_LogsService(logsService),
            m_InputStore(inputStore), m_MouseDeltaStore(mouseDeltaStore) {}
        ~InputService() = default;

        auto OnWindowMessage(WindowMessageHandler handler) -> void;
        auto OnRawMouse(RawMouseHandler handler) -> void;

        auto RaiseWindowMessage(WindowMessage& message) -> bool;
        auto RaiseRawMouse(RawMouse& mouse) -> bool;

        auto BindWindow(HWND window) -> void;

        auto SetAIControlActive(bool active) -> void;

        auto InjectMouseDelta(std::int32_t deltaX, std::int32_t deltaY) -> bool;

        auto SetActionRequested(Action action, bool requested) -> void;

        auto AdvanceInputTick() -> void;

    private:
        LogsService& m_LogsService;
        InputStore& m_InputStore;
        MouseDeltaStore& m_MouseDeltaStore;

        std::atomic<HWND> m_Window{ nullptr };

        std::vector<WindowMessageHandler> m_OnWindowMessage{};
        std::vector<RawMouseHandler> m_OnRawMouse{};
        std::atomic<int> m_HandlerErrors{ 0 };

        auto ReportHandlerError(const char* stage) -> void;
    };
}