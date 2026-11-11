export module Engine.Capture.Oracle.State;

import Engine.Capture.Oracle.Type;
import std;

export namespace Engine::Capture::Oracle::State
{
    // The hand-over of the raycast oracle between the AI thread (it asks for rays and reads the answers) and the game thread (it traces them).
    // note: The game thread never blocks on it: it only tries the lock, and a busy lock means no batch this tick or a dropped outcome.
    class OracleStore
    {
    private:
        using OracleBatch = Engine::Capture::Oracle::Type::OracleBatch;
        using OracleOutcome = Engine::Capture::Oracle::Type::OracleOutcome;
        using OracleStatus = Engine::Capture::Oracle::Type::OracleStatus;

    public:
        OracleStore() = default;
        ~OracleStore() = default;

        OracleStore(const OracleStore&) = delete;
        auto operator=(const OracleStore&) -> OracleStore& = delete;

        // Turns the oracle on or off. Off costs the game thread one atomic read per tick.
        auto SetEnabled(bool value) -> void;
        auto IsEnabled() const -> bool { return m_IsEnabled.load(std::memory_order_relaxed); }

        auto GetStatus() const -> OracleStatus;
        auto SetStatus(OracleStatus value) -> void;

        // Changes the status only if it is still `from`.
        // return: Whether it changed. Lets Start, Stop and Run never overwrite Unavailable or each other.
        auto TryTransition(OracleStatus from, OracleStatus to) -> bool;

        // Counts one tick of the game thread seen with the oracle on.
        // return: The new sequence.
        auto AdvanceSequence() -> std::uint64_t;
        auto GetSequence() const -> std::uint64_t;

        // return: A batch id that no earlier call gave.
        auto AllocateBatchId() -> std::uint64_t;

        // Hands a batch to the game thread, replacing one it has not taken yet (counted as dropped).
        // note: AI thread.
        auto Submit(OracleBatch batch) -> void;

        // return: The pending batch, or nullopt if there is none or the lock is busy.
        // note: Game thread. Never blocks.
        auto TakeBatch() -> std::optional<OracleBatch>;

        // Hands an outcome to the AI thread. Above k_MaxPendingOutcomes the oldest is dropped, and a busy lock drops this one.
        // note: Game thread. Never blocks.
        auto Publish(OracleOutcome outcome) -> void;

        // return: The outcomes published since the last call, oldest first.
        // note: AI thread.
        auto TakeOutcomes() -> std::vector<OracleOutcome>;

        auto GetDroppedBatches() const -> std::uint64_t;
        auto GetDroppedOutcomes() const -> std::uint64_t;

        // Forgets the pending batch and the unread outcomes.
        auto Cleanup() -> void;

    private:
        std::atomic<bool> m_IsEnabled{ false };
        std::atomic<OracleStatus> m_Status{ OracleStatus::Off };
        std::atomic<std::uint64_t> m_Sequence{ 0 };
        std::atomic<std::uint64_t> m_NextBatchId{ 0 };
        std::atomic<std::uint64_t> m_DroppedBatches{ 0 };
        std::atomic<std::uint64_t> m_DroppedOutcomes{ 0 };

        std::mutex m_Mutex{};
        std::optional<OracleBatch> m_Pending{};
        std::deque<OracleOutcome> m_Outcomes{};
    };
}