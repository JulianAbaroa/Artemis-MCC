module;

#include <windows.h>

export module Platform.Input.State:MouseDelta;

import std;

export namespace Platform::Input::State
{
    // Mouse movement injected by the AI, plus the real device handle needed to forge raw input.
    // note: Lock-free. Written by the AI side and read by the GetRawInputData hook.
    class MouseDeltaStore
    {
    public:
        MouseDeltaStore() = default;
        ~MouseDeltaStore() = default;

        // Enables or disables mouse injection.
        auto SetAIControlActive(bool active) -> void;
        auto IsAIControlActive() const -> bool;

        // Accumulates movement to inject on the next raw input read.
        auto AddPendingDelta(std::int32_t deltaX, std::int32_t deltaY) -> void;

        // Takes the accumulated movement and resets it.
        // param outDeltaX: Out. Horizontal movement.
        // param outDeltaY: Out. Vertical movement.
        // return: False if there was no movement.
        auto ConsumePendingDelta(std::int32_t& outDeltaX, std::int32_t& outDeltaY) -> bool;

        // Stores the handle of the real mouse device, seen on genuine raw input.
        // note: Injected input reuses it, so it looks like it came from that device.
        auto SetKnownDeviceHandle(HANDLE device) -> void;
        auto GetKnownDeviceHandle() const -> HANDLE;

        // Clears all state.
        auto Cleanup() -> void;

    private:
        std::atomic<bool> m_AIControlActive{ false };

        std::atomic<std::int32_t> m_PendingDeltaX{ 0 };
        std::atomic<std::int32_t> m_PendingDeltaY{ 0 };

        std::atomic<HANDLE> m_KnownDeviceHandle{ nullptr };
    };
}