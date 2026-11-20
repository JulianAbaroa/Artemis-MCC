export module Engine.Base.Pov.State:Probe;

import Engine.Base.Pov.Type;
import std;

namespace
{
    using Engine::Base::Pov::Type::k_PovProbeMaxThis;
    using Engine::Base::Pov::Type::k_PovProbeRingCapacity;
}

export namespace Engine::Base::Pov::State
{
    // TEMPORAL VW.9a.x (deleted when VW.9a.x closes): what the probe hooks note about who moves the view of user 0, until the game thread
    // writes it to the log. Any thread writes (the hooked functions may run on a thread other than the game thread), the game thread reads.
    // note: The ring has a fixed capacity and never allocates or waits: a writer takes a ticket, fills its slot and publishes the ticket last, and
    // the reader skips a slot whose ticket is not the one it expects (overwritten or not yet published) and counts it as dropped.
    class PovProbeStore
    {
    private:
        using PovProbeCursor = Engine::Base::Pov::Type::PovProbeCursor;
        using PovProbeDrain = Engine::Base::Pov::Type::PovProbeDrain;
        using PovProbeEvent = Engine::Base::Pov::Type::PovProbeEvent;
        using PovProbeKind = Engine::Base::Pov::Type::PovProbeKind;
        using PovProbeSlot = Engine::Base::Pov::Type::PovProbeSlot;

    public:
        PovProbeStore() = default;
        ~PovProbeStore() = default;

        PovProbeStore(const PovProbeStore&) = delete;
        auto operator=(const PovProbeStore&) -> PovProbeStore& = delete;

        // Whether the hooked functions note their calls. Set by the game thread while a POV is requested.
        auto SetActive(bool isActive) -> void;
        auto IsActive() const -> bool;

        // The id of the game thread, so the log can say whether a call came from it.
        auto SetGameThread(std::uint32_t threadId) -> void;
        auto GetGameThread() const -> std::uint32_t;

        // Notes one call. Any thread, no lock, no allocation.
        auto Record(PovProbeKind kind, std::uint32_t threadId, std::uint64_t caller, const std::array<std::uint64_t, 4>& args) -> void;

        // Notes one call of the cursor candidate: counts it, remembers the first distinct this pointers and the last value read. Any thread.
        auto RecordCursor(std::uint64_t thisPointer, std::uint64_t firstValue) -> void;

        // Copies the events not read yet, oldest first, into out.
        // return: How many were copied and how many were lost (overwritten before the read or torn by a writer).
        // note: One reading thread (the game thread).
        auto Drain(std::span<PovProbeEvent> out) -> PovProbeDrain;

        // Forgets the events not read yet. One reading thread.
        auto Discard() -> void;

        // return: The cursor candidate calls since the last take, then starts the count over. One reading thread.
        auto TakeCursor() -> PovProbeCursor;

    private:
        std::array<PovProbeSlot, k_PovProbeRingCapacity> m_Slots{};
        std::atomic<std::uint64_t> m_Head{ 0 };
        std::uint64_t m_Tail{ 0 };

        std::atomic<bool> m_IsActive{ false };
        std::atomic<std::uint32_t> m_GameThread{ 0 };

        std::atomic<std::uint32_t> m_CursorCalls{ 0 };
        std::atomic<std::uint32_t> m_CursorThisCount{ 0 };
        std::array<std::atomic<std::uint64_t>, k_PovProbeMaxThis> m_CursorThis{};
        std::atomic<std::uint64_t> m_CursorFirstValue{ 0 };
    };
}