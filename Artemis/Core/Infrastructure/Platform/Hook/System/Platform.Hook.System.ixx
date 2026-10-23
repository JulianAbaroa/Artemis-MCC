export module Platform.Hook.System;

import Common.Log.System;
import std;

namespace
{
    using Common::Log::System::Log;
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
        void** outOriginal, const char* tag, Log& log) -> bool;

    // Disables the hook but keeps it created, so calls already inside can finish.
    // note: Pair with WaitForDrain and RemoveDetour for a safe removal.
    auto DisableDetour(void* functionAddress) -> void;

    // Removes the hook and logs it.
    auto RemoveDetour(void* functionAddress, const char* tag,
        Log& log) -> void;

    // Overwrites bytes of the game code.
    // param address: First byte to overwrite.
    // param bytes: New bytes.
    // param outOriginal: Out. The bytes that were there, for RestorePatch.
    // param tag: Log tag of the caller, such as "[SessionJoinCheckPatch]".
    // return: False if the address is null or the memory cannot be made writable. The cause is logged.
    auto WritePatch(void* address, std::span<const std::uint8_t> bytes,
        std::vector<std::uint8_t>& outOriginal, const char* tag, Log& log) -> bool;

    // Writes back the bytes saved by WritePatch.
    auto RestorePatch(void* address, std::span<const std::uint8_t> original,
        const char* tag, Log& log) -> void;

    // Counts one call inside a hooked function for as long as it lives.
    // note: Lets WaitForDrain know when the hook can be removed safely. A thread that entered the detour but has not built the scope
    // yet is not counted: the window is a few instructions and every hook shares it.
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

    // return: The address a RIP-relative operand inside a signature match points to: the instruction that ends at nextOffset, plus
    // the rel32 or disp32 at relOffset.
    auto ResolveRelative(std::uintptr_t match, std::uintptr_t relOffset, std::uintptr_t nextOffset) -> std::uintptr_t;

    // return: Address of the engine global that holds the TLS slot of the thread context, resolved from the match of
    // Signature::k_TelemetryIdModifier, or nullptr if there is no match. The slot is read through it on every call.
    // note: Reads the code bytes of the match. The caller guards it with __try if the match can be stale.
    auto ResolveTlsIndex(std::uintptr_t match) -> const std::uint32_t*;

    // return: The engine thread context of the current thread, or 0 if the slot is out of range or holds no context.
    // note: The context lives in the thread local storage of the threads that run the engine (the game thread). Reads raw memory:
    // the caller guards it with __try where the slot can be wrong.
    auto ReadThreadContext(std::uint32_t tlsIndex) -> std::uintptr_t;

    // Longest wait for the calls in flight when a hook is removed.
    inline constexpr std::chrono::milliseconds k_DrainTimeout{ 500 };

    // Waits until the counter reaches zero.
    // return: False if the timeout expired with calls still in flight.
    auto WaitForDrain(std::atomic<int>& counter,
        std::chrono::milliseconds timeout) -> bool;
}