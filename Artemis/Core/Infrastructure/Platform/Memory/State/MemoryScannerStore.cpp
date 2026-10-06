module Platform.Memory.State;
import :Scanner;

namespace
{
    using DataType = Platform::Memory::Type::DataType;
}

namespace Platform::Memory::State
{
    auto MemoryScannerStore::GetSession() const -> const Session&
    {
        return m_Session;
    }

    auto MemoryScannerStore::GetFilter() const -> const Filter&
    {
        return m_Session.Filter;
    }

    auto MemoryScannerStore::IsScanning() const -> bool
    {
        return m_IsScanning;
    }

    auto MemoryScannerStore::SetFilter(const Filter& filter) -> void
    {
        m_Session.Filter = filter;
    }

    auto MemoryScannerStore::SetRegion(const std::string& name, std::uintptr_t base, std::size_t size) -> void
    {
        m_Session.Region.Name = name;
        m_Session.Region.BaseAddress = base;
        m_Session.Region.Size = size;
    }

    auto MemoryScannerStore::SetScanning(bool isScanning) -> void
    {
        m_IsScanning = isScanning;
    }

    auto MemoryScannerStore::BeginRound() -> void
    {
        m_Session.Rounds.push_back(Round{});
    }

    auto MemoryScannerStore::BeginUnchangedRound() -> void
    {
        Round round{};
        round.IsUnchangedRound = true;
        m_Session.Rounds.push_back(std::move(round));
    }

    auto MemoryScannerStore::SetRoundBefore(Snapshot snapshot) -> void
    {
        if (m_Session.Rounds.empty()) return;

        m_Session.Rounds.back().Before = std::move(snapshot);
    }

    auto MemoryScannerStore::SetRoundAfter(Snapshot snapshot) -> void
    {
        if (m_Session.Rounds.empty()) return;

        m_Session.Rounds.back().After = std::move(snapshot);
    }

    auto MemoryScannerStore::SetRoundDiffs(std::vector<ByteDiff> diffs) -> void
    {
        if (m_Session.Rounds.empty()) return;

        m_Session.Rounds.back().Diffs = std::move(diffs);
    }

    auto MemoryScannerStore::SetRoundTypedDiffs(std::vector<TypedMatch> matches) -> void
    {
        if (m_Session.Rounds.empty()) return;

        m_Session.Rounds.back().TypedDiffs = std::move(matches);
    }

    auto MemoryScannerStore::CompleteRound() -> void
    {
        if (m_Session.Rounds.empty()) return;

        m_Session.Rounds.back().IsComplete = true;
    }

    auto MemoryScannerStore::ComputeFinalDiffs() -> void
    {
        const bool isTyped = (m_Session.Filter.DataType != DataType::Bytes);

        if (!isTyped)
        {
            std::vector<const Round*> normalRounds{};
            std::vector<const Round*> unchangedRounds{};

            for (const auto& round : m_Session.Rounds)
            {
                if (!round.IsComplete) continue;

                (round.IsUnchangedRound ? unchangedRounds : normalRounds).push_back(&round);
            }

            if (normalRounds.empty())
            {
                m_Session.FinalDiffs.clear();
                return;
            }

            // Keeps the diffs that appear identical in every normal round.
            std::vector<ByteDiff> result = normalRounds[0]->Diffs;

            for (std::size_t i = 1; i < normalRounds.size(); ++i)
            {
                std::vector<ByteDiff> intersection{};

                for (const auto& a : result)
                {
                    for (const auto& b : normalRounds[i]->Diffs)
                    {
                        if (a.Offset == b.Offset && a.Before == b.Before && a.After == b.After)
                        {
                            intersection.push_back(a);
                            break;
                        }
                    }
                }

                result = std::move(intersection);
            }

            // Drops the offsets that changed in any unchanged round.
            for (const auto* unchangedRound : unchangedRounds)
            {
                std::vector<ByteDiff> filtered{};

                for (const auto& a : result)
                {
                    bool hasChanged = false;

                    for (const auto& b : unchangedRound->Diffs)
                    {
                        if (a.Offset == b.Offset)
                        {
                            hasChanged = true;
                            break;
                        }
                    }

                    if (!hasChanged) filtered.push_back(a);
                }

                result = std::move(filtered);
            }

            m_Session.FinalDiffs = std::move(result);
        }
        else
        {
            // Typed modes ignore the unchanged rounds.
            std::vector<const Round*> rounds{};

            for (const auto& round : m_Session.Rounds)
            {
                if (round.IsComplete && !round.IsUnchangedRound) rounds.push_back(&round);
            }

            if (rounds.empty())
            {
                m_Session.FinalMatches.clear();
                return;
            }

            // Keeps the offsets present in every round, with the value of the last one.
            std::vector<TypedMatch> result = rounds[0]->TypedDiffs;

            for (std::size_t i = 1; i < rounds.size(); ++i)
            {
                std::vector<TypedMatch> intersection{};

                for (const auto& a : result)
                {
                    for (const auto& b : rounds[i]->TypedDiffs)
                    {
                        if (a.Offset == b.Offset)
                        {
                            intersection.push_back(b);
                            break;
                        }
                    }
                }

                result = std::move(intersection);
            }

            m_Session.FinalMatches = std::move(result);
        }
    }

    auto MemoryScannerStore::Reset() -> void
    {
        m_Session = Session{};
        m_IsScanning = false;
    }
}