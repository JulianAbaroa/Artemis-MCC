export module Engine.Capture.State;

import Blam.Layout.Snapshot.Type;
import std;

export namespace Engine::Capture::State
{
    // Holds the addresses the capture copies from and the last published snapshot.
    // note: Lock-free. A published snapshot is never modified.
    class CaptureStore
    {
    private:
        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;

    public:
        CaptureStore() = default;
        ~CaptureStore() = default;

        // return: Engine thread context that leads to the globals and the map variant, or 0 if it was not found yet.
        auto GetThreadContext() const -> std::uintptr_t;
        auto SetThreadContext(std::uintptr_t pointer) -> void;

        // return: Address of the object table, or 0 if it was not found yet.
        auto GetObjectTableBase() const -> std::uintptr_t;
        auto SetObjectTableBase(std::uintptr_t pointer) -> void;

        // return: Address of the player table, or 0 if it was not found yet.
        auto GetPlayerTableBase() const -> std::uintptr_t;
        auto SetPlayerTableBase(std::uintptr_t pointer) -> void;

        // return: Address of the player mapping globals, or 0 if they were not found yet.
        auto GetMappingBase() const -> std::uintptr_t;
        auto SetMappingBase(std::uintptr_t pointer) -> void;

        // return: Address of the first output user entry of the interaction tables, or 0 if it was not found yet.
        auto GetInteractionTableBase() const -> std::uintptr_t;
        auto SetInteractionTableBase(std::uintptr_t pointer) -> void;

        // Replaces the snapshot that Acquire returns.
        auto Publish(std::shared_ptr<const Snapshot> snapshot) -> void;

        // return: Last published snapshot, or null before the first one and after Cleanup.
        auto Acquire() const -> std::shared_ptr<const Snapshot>;

        // Clears every address and drops the published snapshot.
        auto Cleanup() -> void;

    private:
        std::atomic<std::uintptr_t> m_ThreadContext{ 0 };
        std::atomic<std::uintptr_t> m_ObjectTableBase{ 0 };
        std::atomic<std::uintptr_t> m_PlayerTableBase{ 0 };
        std::atomic<std::uintptr_t> m_MappingBase{ 0 };
        std::atomic<std::uintptr_t> m_InteractionTableBase{ 0 };
        std::atomic<std::shared_ptr<const Snapshot>> m_Snapshot{};
    };
}