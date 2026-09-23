module Platform.Memory.State;
import :Scanner;

namespace
{
    using DataType = Platform::Memory::Type::DataType;
}

namespace Platform::Memory::State
{
    auto MemoryScannerStore::SetRegion(const std::string& name, std::uintptr_t base, std::size_t size) -> void
    {
        m_Session.Region.Name = name;
        m_Session.Region.BaseAddress = base;
        m_Session.Region.Size = size;
    }

    auto MemoryScannerStore::SetFilter(const Filter& filter) -> void
    {
        m_Session.Filter = filter;
    }

    auto MemoryScannerStore::SetDelayMs(int ms) -> void
    {
        m_DelayMs = ms;
    }

    auto MemoryScannerStore::SetWaitingForKey(bool w) -> void
    {
        m_WaitingForKey = w;
    }

    auto MemoryScannerStore::SetScanning(bool s) -> void
    {
        m_Scanning = s;
    }

    auto MemoryScannerStore::BeginRound() -> void
    {
        Round round;
        round.IsUnchangedRound = false;
        m_Session.Rounds.push_back(round);
    }

    auto MemoryScannerStore::BeginUnchangedRound() -> void
    {
        Round round;
        round.IsUnchangedRound = true;
        m_Session.Rounds.push_back(round);
    }

    auto MemoryScannerStore::SetRoundBefore(Snapshot snap) -> void
    {
        if (m_Session.Rounds.empty()) return;
        m_Session.Rounds.back().Before = std::move(snap);
    }

    auto MemoryScannerStore::SetRoundAfter(Snapshot snap) -> void
    {
        if (m_Session.Rounds.empty()) return;
        m_Session.Rounds.back().After = std::move(snap);
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
            // Byte-by-byte modes
            std::vector<const Round*> normalRounds, unchangedRounds;
            for (const auto& r : m_Session.Rounds)
            {
                if (!r.IsComplete) continue;
                (r.IsUnchangedRound ? unchangedRounds : normalRounds).push_back(&r);
            }

            if (normalRounds.empty()) { m_Session.FinalDiffs.clear(); return; }

            std::vector<ByteDiff> result = normalRounds[0]->Diffs;

            for (std::size_t i = 1; i < normalRounds.size(); ++i)
            {
                std::vector<ByteDiff> inter;
                for (const auto& a : result)
                    for (const auto& b : normalRounds[i]->Diffs)
                        if (a.Offset == b.Offset && a.Before == b.Before && a.After == b.After)
                        {
                            inter.push_back(a); break;
                        }
                result = std::move(inter);
            }

            for (const auto* u : unchangedRounds)
            {
                std::vector<ByteDiff> filtered;
                for (const auto& a : result)
                {
                    bool changed = false;
                    for (const auto& b : u->Diffs)
                        if (a.Offset == b.Offset) { changed = true; break; }
                    if (!changed) filtered.push_back(a);
                }
                result = std::move(filtered);
            }

            m_Session.FinalDiffs = std::move(result);
        }
        else
        {
            // Intersection of typical roundabouts
            // Normal rounds only (IsUnchangedRound does not apply in typed modes).
            std::vector<const Round*> rounds;
            for (const auto& r : m_Session.Rounds)
                if (r.IsComplete && !r.IsUnchangedRound) rounds.push_back(&r);

            if (rounds.empty()) { m_Session.FinalMatches.clear(); return; }

            std::vector<TypedMatch> result = rounds[0]->TypedDiffs;

            for (std::size_t i = 1; i < rounds.size(); ++i)
            {
                std::vector<TypedMatch> inter;
                for (const auto& a : result)
                    for (const auto& b : rounds[i]->TypedDiffs)
                        if (a.Offset == b.Offset) { inter.push_back(b); break; } // actualiza After
                result = std::move(inter);
            }

            m_Session.FinalMatches = std::move(result);
        }
    }

    auto MemoryScannerStore::Reset() -> void
    {
        m_Session = Session{};
        m_Scanning = false;
    }

    auto MemoryScannerStore::GetSession() const -> const Session&
    {
        return m_Session;
    }

    auto MemoryScannerStore::GetFilter() const -> const Filter&
    {
        return m_Session.Filter;
    }

    auto MemoryScannerStore::GetDelayMs() const -> int
    {
        return m_DelayMs;
    }

    auto MemoryScannerStore::IsWaitingForKey() const -> bool
    {
        return m_WaitingForKey;
    }

    auto MemoryScannerStore::IsScanning() const -> bool
    {
        return m_Scanning;
    }
}