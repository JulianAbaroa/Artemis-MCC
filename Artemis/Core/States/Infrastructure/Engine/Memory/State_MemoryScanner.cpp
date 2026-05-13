#include "pch.h"

// Header.
#include "State_MemoryScanner.h"

void State_MemoryScanner::SetRegion(const std::string& name, uintptr_t base, size_t size)
{
    m_Session.Region.Name = name;
    m_Session.Region.BaseAddress = base;
    m_Session.Region.Size = size;
}

void State_MemoryScanner::SetFilter(const ScanFilter& filter)
{
    m_Session.Filter = filter;
}

void State_MemoryScanner::SetDelayMs(int ms) { m_DelayMs = ms; }
void State_MemoryScanner::SetWaitingForKey(bool w) { m_WaitingForKey = w; }
void State_MemoryScanner::SetBoundKey(int key) { m_BoundKey = key; m_WaitingForKey = false; }
void State_MemoryScanner::SetScanning(bool s) { m_Scanning = s; }

void State_MemoryScanner::BeginRound()
{
    ScanRound round;
    round.IsUnchangedRound = false;
    m_Session.Rounds.push_back(round);
}

void State_MemoryScanner::BeginUnchangedRound()
{
    ScanRound round;
    round.IsUnchangedRound = true;
    m_Session.Rounds.push_back(round);
}

void State_MemoryScanner::SetRoundBefore(ScanSnapshot snap)
{
    if (m_Session.Rounds.empty()) return;
    m_Session.Rounds.back().Before = std::move(snap);
}

void State_MemoryScanner::SetRoundAfter(ScanSnapshot snap)
{
    if (m_Session.Rounds.empty()) return;
    m_Session.Rounds.back().After = std::move(snap);
}

void State_MemoryScanner::SetRoundDiffs(std::vector<ByteDiff> diffs)
{
    if (m_Session.Rounds.empty()) return;
    m_Session.Rounds.back().Diffs = std::move(diffs);
}

void State_MemoryScanner::SetRoundTypedDiffs(std::vector<TypedMatch> matches)
{
    if (m_Session.Rounds.empty()) return;
    m_Session.Rounds.back().TypedDiffs = std::move(matches);
}

void State_MemoryScanner::CompleteRound()
{
    if (m_Session.Rounds.empty()) return;
    m_Session.Rounds.back().IsComplete = true;
}

void State_MemoryScanner::ComputeFinalDiffs()
{
    const bool isTyped = (m_Session.Filter.DataType != ScanDataType::Bytes);

    if (!isTyped)
    {
        // Byte-by-byte modes
        std::vector<const ScanRound*> normalRounds, unchangedRounds;
        for (const auto& r : m_Session.Rounds)
        {
            if (!r.IsComplete) continue;
            (r.IsUnchangedRound ? unchangedRounds : normalRounds).push_back(&r);
        }

        if (normalRounds.empty()) { m_Session.FinalDiffs.clear(); return; }

        std::vector<ByteDiff> result = normalRounds[0]->Diffs;

        for (size_t i = 1; i < normalRounds.size(); ++i)
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
        std::vector<const ScanRound*> rounds;
        for (const auto& r : m_Session.Rounds)
            if (r.IsComplete && !r.IsUnchangedRound) rounds.push_back(&r);

        if (rounds.empty()) { m_Session.FinalMatches.clear(); return; }

        std::vector<TypedMatch> result = rounds[0]->TypedDiffs;

        for (size_t i = 1; i < rounds.size(); ++i)
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

void State_MemoryScanner::Reset()
{
    m_Session = ScanSession{};
    m_Scanning = false;
}

const ScanSession& State_MemoryScanner::GetSession() const { return m_Session; }
const ScanFilter& State_MemoryScanner::GetFilter()  const { return m_Session.Filter; }
int  State_MemoryScanner::GetDelayMs()    const { return m_DelayMs; }
int  State_MemoryScanner::GetBoundKey()   const { return m_BoundKey; }
bool State_MemoryScanner::IsWaitingForKey() const { return m_WaitingForKey; }
bool State_MemoryScanner::IsScanning()    const { return m_Scanning; }