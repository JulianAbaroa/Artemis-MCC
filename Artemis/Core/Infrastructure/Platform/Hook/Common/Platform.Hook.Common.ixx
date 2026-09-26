export module Platform.Hook.Common;

import Service.Logs.System;
import std;

export namespace Platform::Hook::Common
{
    using LogsService = Service::Logs::System::LogsService;

    auto InstallDetour(void* functionAddress, void* hookedFunction,
        void** outOriginal, const char* tag, LogsService& logsService) -> bool;

    auto DisableDetour(void* functionAddress) -> void;

    auto RemoveDetour(void* functionAddress, const char* tag,
        LogsService& logsService) -> void;

    auto UninstallDetour(void* functionAddress, const char* tag,
        LogsService& logsService) -> void;

    class InFlightScope
    {
    public:
        explicit InFlightScope(std::atomic<int>& counter) : m_Counter(counter)
        {
            m_Counter.fetch_add(1, std::memory_order_acq_rel);
        }
        ~InFlightScope()
        {
            m_Counter.fetch_sub(1, std::memory_order_acq_rel);
        }

        InFlightScope(const InFlightScope&) = delete;
        InFlightScope& operator=(const InFlightScope&) = delete;

    private:
        std::atomic<int>& m_Counter;
    };

    auto WaitForDrain(std::atomic<int>& counter,
        std::chrono::milliseconds timeout) -> bool;
}