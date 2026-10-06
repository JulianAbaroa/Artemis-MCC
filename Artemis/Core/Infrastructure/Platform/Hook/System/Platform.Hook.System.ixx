export module Platform.Hook.System;

import Service.Logs.System;
import std;

namespace
{
    using Service::Logs::System::LogsService;
}

export namespace Platform::Hook::System
{
    // Hooks a function with MinHook and enables the hook. Any previous hook on that address is removed first.
    // param functionAddress: Function to hook.
    // param hookedFunction: Replacement that runs instead.
    // param outOriginal: Receives the pointer used to call the original function.
    // param tag: Log tag of the caller, such as "[PresentDetour]".
    // return: False if the address is null or MinHook fails. The cause is logged.
    auto InstallDetour(void* functionAddress, void* hookedFunction,
        void** outOriginal, const char* tag, LogsService& logsService) -> bool;

    // Disables the hook but keeps it created, so calls already inside can finish.
    // note: Pair with WaitForDrain and RemoveDetour for a safe removal.
    auto DisableDetour(void* functionAddress) -> void;

    // Removes the hook and logs it.
    auto RemoveDetour(void* functionAddress, const char* tag,
        LogsService& logsService) -> void;

    // Disables and removes the hook. For hooks that don't track calls in flight.
    auto UninstallDetour(void* functionAddress, const char* tag,
        LogsService& logsService) -> void;

    // Counts one call inside a hooked function for as long as it lives.
    // note: Lets WaitForDrain know when the hook can be removed safely.
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
        auto operator=(const InFlightScope&) -> InFlightScope& = delete;

    private:
        std::atomic<int>& m_Counter;
    };

    // Waits until the counter reaches zero.
    // return: False if the timeout expired with calls still in flight.
    auto WaitForDrain(std::atomic<int>& counter,
        std::chrono::milliseconds timeout) -> bool;
}