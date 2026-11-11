module Engine.Capture.Oracle.State;

import std;

namespace
{
    using Engine::Capture::Oracle::Type::k_MaxPendingOutcomes;
}

namespace Engine::Capture::Oracle::State
{
    auto OracleStore::SetEnabled(bool value) -> void
    {
        m_IsEnabled.store(value, std::memory_order_relaxed);
    }

    auto OracleStore::GetStatus() const -> OracleStatus
    {
        return m_Status.load();
    }

    auto OracleStore::SetStatus(OracleStatus value) -> void
    {
        m_Status.store(value);
    }

    auto OracleStore::TryTransition(OracleStatus from, OracleStatus to) -> bool
    {
        return m_Status.compare_exchange_strong(from, to);
    }

    auto OracleStore::AdvanceSequence() -> std::uint64_t
    {
        return m_Sequence.fetch_add(1) + 1;
    }

    auto OracleStore::GetSequence() const -> std::uint64_t
    {
        return m_Sequence.load();
    }

    auto OracleStore::AllocateBatchId() -> std::uint64_t
    {
        return m_NextBatchId.fetch_add(1) + 1;
    }

    auto OracleStore::Submit(OracleBatch batch) -> void
    {
        std::lock_guard lock{ m_Mutex };

        if (m_Pending.has_value()) m_DroppedBatches.fetch_add(1);
        m_Pending = std::move(batch);
    }

    auto OracleStore::TakeBatch() -> std::optional<OracleBatch>
    {
        std::unique_lock lock{ m_Mutex, std::try_to_lock };
        if (!lock.owns_lock() || !m_Pending.has_value()) return std::nullopt;

        std::optional<OracleBatch> batch{ std::move(m_Pending) };
        m_Pending.reset();
        return batch;
    }

    auto OracleStore::Publish(OracleOutcome outcome) -> void
    {
        std::unique_lock lock{ m_Mutex, std::try_to_lock };
        if (!lock.owns_lock())
        {
            m_DroppedOutcomes.fetch_add(1);
            return;
        }

        m_Outcomes.push_back(std::move(outcome));
        while (m_Outcomes.size() > k_MaxPendingOutcomes)
        {
            m_Outcomes.pop_front();
            m_DroppedOutcomes.fetch_add(1);
        }
    }

    auto OracleStore::TakeOutcomes() -> std::vector<OracleOutcome>
    {
        std::lock_guard lock{ m_Mutex };

        std::vector<OracleOutcome> outcomes{ std::make_move_iterator(m_Outcomes.begin()), std::make_move_iterator(m_Outcomes.end()) };
        m_Outcomes.clear();
        return outcomes;
    }

    auto OracleStore::GetDroppedBatches() const -> std::uint64_t
    {
        return m_DroppedBatches.load();
    }

    auto OracleStore::GetDroppedOutcomes() const -> std::uint64_t
    {
        return m_DroppedOutcomes.load();
    }

    auto OracleStore::Cleanup() -> void
    {
        std::lock_guard lock{ m_Mutex };

        m_Pending.reset();
        m_Outcomes.clear();
    }
}