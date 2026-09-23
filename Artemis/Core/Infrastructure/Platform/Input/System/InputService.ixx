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
        using Request = Platform::Input::Type::Request;
        using WindowMessage = Platform::Input::Type::WindowMessage;
        using RawMouse = Platform::Input::Type::RawMouse;
        using Milliseconds = std::chrono::milliseconds;

        using LogsService = Service::Logs::System::LogsService;
        using InputStore = Platform::Input::State::InputStore;

    public:
        using WindowMessageHandler = std::function<bool(WindowMessage&)>;
        using RawMouseHandler = std::function<bool(RawMouse&)>;

        InputService(LogsService& logsService, InputStore& inputStore) :
            m_LogsService(logsService), m_InputStore(inputStore) {}
        ~InputService() = default;

        auto AutomaticInput() -> void;

        auto OnWindowMessage(WindowMessageHandler handler) -> void;
        auto OnRawMouse(RawMouseHandler handler) -> void;

        auto RaiseWindowMessage(WindowMessage& message) -> bool;
        auto RaiseRawMouse(RawMouse& mouse) -> bool;

    private:
        LogsService& m_LogsService;
        InputStore& m_InputStore;

        std::vector<WindowMessageHandler> m_OnWindowMessage{};
        std::vector<RawMouseHandler> m_OnRawMouse{};
        std::atomic<int> m_HandlerErrors{ 0 };

        auto ReportHandlerError(const char* stage) -> void;

        auto InjectInput(Request req, std::function<bool()> successCondition,
            Milliseconds timeoutMs, Milliseconds stabilizeMs) -> bool;
    };
}