#include "pch.h"

// Header.
#include "System_MemoryScanner.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

// Memory Scanner.
#include "Core/States/Infrastructure/Engine/Memory/State_MemoryScanner.h"

#include <algorithm>
#include <cstring>
#include <bit>

System_MemoryScanner::System_MemoryScanner() = default;
System_MemoryScanner::~System_MemoryScanner()
{
    if (m_WorkerThread.joinable()) m_WorkerThread.join();
}

// Helpers.

template<typename T>
T System_MemoryScanner::ReadAt(const std::vector<uint8_t>& buf, size_t offset) const
{
    T val{};
    std::memcpy(&val, buf.data() + offset, sizeof(T));
    return val;
}

// Extract the raw value as uint64_t from the buffer at the given offset
static uint64_t ExtractRaw(const std::vector<uint8_t>& buf, size_t offset, ScanDataType type)
{
    switch (type)
    {
    case ScanDataType::Int8: { int8_t  v; std::memcpy(&v, buf.data() + offset, 1); return static_cast<uint64_t>(v); }
    case ScanDataType::UInt8: { uint8_t v; std::memcpy(&v, buf.data() + offset, 1); return v; }
    case ScanDataType::Int16: { int16_t  v; std::memcpy(&v, buf.data() + offset, 2); return static_cast<uint64_t>(v); }
    case ScanDataType::UInt16: { uint16_t v; std::memcpy(&v, buf.data() + offset, 2); return v; }
    case ScanDataType::Int32: { int32_t  v; std::memcpy(&v, buf.data() + offset, 4); return static_cast<uint64_t>(v); }
    case ScanDataType::UInt32: { uint32_t v; std::memcpy(&v, buf.data() + offset, 4); return v; }
    case ScanDataType::Float32: { uint32_t v; std::memcpy(&v, buf.data() + offset, 4); return v; }
    default: return buf[offset];
    }
}

bool System_MemoryScanner::MatchesPair(uint64_t before, uint64_t after, const ScanFilter& f) const
{
    switch (f.Mode)
    {
    case ScanMode::Changed:       return before != after;
    case ScanMode::Unchanged:     return before == after;
    case ScanMode::Increased:     return after > before;
    case ScanMode::Decreased:     return after < before;
    case ScanMode::IncreasedBy:   return (after - before) == f.ValueA;
    case ScanMode::DecreasedBy:   return (before - after) == f.ValueA;

    case ScanMode::ExactValue:
        // In refinement: the current value must be the expected value
        return after == f.ValueA;

    case ScanMode::InRange:
        return after >= f.ValueA && after <= f.ValueB;

    case ScanMode::BitMask:
        return (static_cast<uint32_t>(after) & f.BitMaskPattern) == f.BitMaskValue;

    case ScanMode::Stabilized:
        // It is handled at the round level in ComputeFinalDiffs
        return before == after;

    default: return false;
    }
}

// Base differential scan

void System_MemoryScanner::RunDifferentialScan(int delayMs)
{
    CaptureSnapshot(true);

    if (m_WorkerThread.joinable()) m_WorkerThread.join();

    m_WorkerThread = std::thread([this, delayMs]() {
        Sleep(delayMs);
        this->CaptureSnapshot(false);

        const auto& filter = g_pState->Infrastructure->MemoryScanner->GetFilter();
        if (filter.DataType == ScanDataType::Bytes)
            this->ComputeRoundDiff();
        else
            this->ComputeTypedRoundDiff();

        g_pState->Infrastructure->MemoryScanner->ComputeFinalDiffs();
        g_pState->Infrastructure->MemoryScanner->SetScanning(false);
        });
}

// Public API

void System_MemoryScanner::SetRegion(const std::string& name, uintptr_t base, size_t size)
{
    g_pState->Infrastructure->MemoryScanner->SetRegion(name, base, size);
}

void System_MemoryScanner::TriggerScan(int delayMs)
{
    if (g_pState->Infrastructure->MemoryScanner->IsScanning()) return;
    g_pState->Infrastructure->MemoryScanner->SetScanning(true);

    ScanFilter f; f.Mode = ScanMode::Changed; f.DataType = ScanDataType::Bytes;
    g_pState->Infrastructure->MemoryScanner->SetFilter(f);
    g_pState->Infrastructure->MemoryScanner->BeginRound();
    RunDifferentialScan(delayMs);
}

void System_MemoryScanner::TriggerUnchangedScan(int delayMs)
{
    if (g_pState->Infrastructure->MemoryScanner->IsScanning()) return;
    g_pState->Infrastructure->MemoryScanner->SetScanning(true);

    ScanFilter f; f.Mode = ScanMode::Unchanged; f.DataType = ScanDataType::Bytes;
    g_pState->Infrastructure->MemoryScanner->SetFilter(f);
    g_pState->Infrastructure->MemoryScanner->BeginUnchangedRound();
    RunDifferentialScan(delayMs);
}

void System_MemoryScanner::TriggerIncreasedScan(ScanDataType type, int delayMs)
{
    if (g_pState->Infrastructure->MemoryScanner->IsScanning()) return;
    g_pState->Infrastructure->MemoryScanner->SetScanning(true);

    ScanFilter f; f.Mode = ScanMode::Increased; f.DataType = type;
    g_pState->Infrastructure->MemoryScanner->SetFilter(f);
    g_pState->Infrastructure->MemoryScanner->BeginRound();
    RunDifferentialScan(delayMs);
}

void System_MemoryScanner::TriggerDecreasedScan(ScanDataType type, int delayMs)
{
    if (g_pState->Infrastructure->MemoryScanner->IsScanning()) return;
    g_pState->Infrastructure->MemoryScanner->SetScanning(true);

    ScanFilter f; f.Mode = ScanMode::Decreased; f.DataType = type;
    g_pState->Infrastructure->MemoryScanner->SetFilter(f);
    g_pState->Infrastructure->MemoryScanner->BeginRound();
    RunDifferentialScan(delayMs);
}

void System_MemoryScanner::TriggerIncreasedByScan(ScanDataType type, uint64_t delta, int delayMs)
{
    if (g_pState->Infrastructure->MemoryScanner->IsScanning()) return;
    g_pState->Infrastructure->MemoryScanner->SetScanning(true);

    ScanFilter f; f.Mode = ScanMode::IncreasedBy; f.DataType = type; f.ValueA = delta;
    g_pState->Infrastructure->MemoryScanner->SetFilter(f);
    g_pState->Infrastructure->MemoryScanner->BeginRound();
    RunDifferentialScan(delayMs);
}

void System_MemoryScanner::TriggerDecreasedByScan(ScanDataType type, uint64_t delta, int delayMs)
{
    if (g_pState->Infrastructure->MemoryScanner->IsScanning()) return;
    g_pState->Infrastructure->MemoryScanner->SetScanning(true);

    ScanFilter f; f.Mode = ScanMode::DecreasedBy; f.DataType = type; f.ValueA = delta;
    g_pState->Infrastructure->MemoryScanner->SetFilter(f);
    g_pState->Infrastructure->MemoryScanner->BeginRound();
    RunDifferentialScan(delayMs);
}

// Exact Value, first pass: no need for Before, just the current snapshot
void System_MemoryScanner::TriggerExactScan(ScanDataType type, uint64_t value, int delayMs)
{
    if (g_pState->Infrastructure->MemoryScanner->IsScanning()) return;
    g_pState->Infrastructure->MemoryScanner->SetScanning(true);

    ScanFilter f; f.Mode = ScanMode::ExactValue; f.DataType = type; f.ValueA = value;
    g_pState->Infrastructure->MemoryScanner->SetFilter(f);
    g_pState->Infrastructure->MemoryScanner->BeginRound();

    // We use Before as a single snapshot
    CaptureSnapshot(true); 

    if (m_WorkerThread.joinable()) m_WorkerThread.join();
    m_WorkerThread = std::thread([this, f]() {
        Sleep(0);
        this->ComputeExactMatchFirstPass(f);
        g_pState->Infrastructure->MemoryScanner->ComputeFinalDiffs();
        g_pState->Infrastructure->MemoryScanner->SetScanning(false);
        });
}

void System_MemoryScanner::TriggerExactScanFloat(float value, int delayMs)
{
    uint32_t raw; std::memcpy(&raw, &value, 4);
    TriggerExactScan(ScanDataType::Float32, raw, delayMs);
}

void System_MemoryScanner::TriggerInRangeScan(ScanDataType type, uint64_t lo, uint64_t hi, int delayMs)
{
    if (g_pState->Infrastructure->MemoryScanner->IsScanning()) return;
    g_pState->Infrastructure->MemoryScanner->SetScanning(true);

    ScanFilter f; f.Mode = ScanMode::InRange; f.DataType = type; f.ValueA = lo; f.ValueB = hi;
    g_pState->Infrastructure->MemoryScanner->SetFilter(f);
    g_pState->Infrastructure->MemoryScanner->BeginRound();
    RunDifferentialScan(delayMs);
}

void System_MemoryScanner::TriggerInRangeScanFloat(float lo, float hi, int delayMs)
{
    uint32_t rlo, rhi;
    std::memcpy(&rlo, &lo, 4);
    std::memcpy(&rhi, &hi, 4);
    TriggerInRangeScan(ScanDataType::Float32, rlo, rhi, delayMs);
}

void System_MemoryScanner::TriggerBitMaskScan(uint32_t mask, uint32_t pattern, int delayMs)
{
    if (g_pState->Infrastructure->MemoryScanner->IsScanning()) return;
    g_pState->Infrastructure->MemoryScanner->SetScanning(true);

    ScanFilter f;
    f.Mode = ScanMode::BitMask;
    f.DataType = ScanDataType::UInt32;
    f.BitMaskPattern = mask;
    f.BitMaskValue = pattern;
    g_pState->Infrastructure->MemoryScanner->SetFilter(f);
    g_pState->Infrastructure->MemoryScanner->BeginRound();
    RunDifferentialScan(delayMs);
}

void System_MemoryScanner::TriggerStabilizedScan(ScanDataType type, int rounds, int delayMs)
{
    // Stabilized: Triggers N rounds of typed "Unchanged" and then ComputeFinalDiffs filters them.
    // Each call to TriggerStabilizedScan adds one round, the caller calls it N times.
    if (g_pState->Infrastructure->MemoryScanner->IsScanning()) return;
    g_pState->Infrastructure->MemoryScanner->SetScanning(true);

    ScanFilter f; f.Mode = ScanMode::Stabilized; f.DataType = type; f.StabilizeRounds = rounds;
    g_pState->Infrastructure->MemoryScanner->SetFilter(f);
    g_pState->Infrastructure->MemoryScanner->BeginRound();
    RunDifferentialScan(delayMs);
}

void System_MemoryScanner::Reset()
{
    if (m_WorkerThread.joinable()) m_WorkerThread.join();
    g_pState->Infrastructure->MemoryScanner->SetScanning(false);
    g_pState->Infrastructure->MemoryScanner->Reset();
}

// Diff logic

void System_MemoryScanner::CaptureSnapshot(bool isBefore)
{
    const auto& region = g_pState->Infrastructure->MemoryScanner->GetSession().Region;
    if (region.BaseAddress == 0 || region.Size == 0) return;

    ScanSnapshot snap;
    snap.Data = ReadMemory(region.BaseAddress, region.Size);
    snap.Label = isBefore ? "Before" : "After";
    snap.BaseAddress = region.BaseAddress;

    if (isBefore) g_pState->Infrastructure->MemoryScanner->SetRoundBefore(std::move(snap));
    else          g_pState->Infrastructure->MemoryScanner->SetRoundAfter(std::move(snap));
}

void System_MemoryScanner::ComputeRoundDiff()
{
    const auto& session = g_pState->Infrastructure->MemoryScanner->GetSession();
    if (session.Rounds.empty()) return;

    const auto& round = session.Rounds.back();
    const auto& before = round.Before.Data;
    const auto& after = round.After.Data;
    size_t count = (std::min)(before.size(), after.size());

    std::vector<ByteDiff> diffs;
    for (size_t i = 0; i < count; ++i)
        if (before[i] != after[i])
            diffs.push_back({ i, before[i], after[i] });

    g_pState->Infrastructure->MemoryScanner->SetRoundDiffs(std::move(diffs));
    g_pState->Infrastructure->MemoryScanner->CompleteRound();
}

void System_MemoryScanner::ComputeTypedRoundDiff()
{
    const auto& session = g_pState->Infrastructure->MemoryScanner->GetSession();
    if (session.Rounds.empty()) return;

    const auto& round = session.Rounds.back();
    const auto& before = round.Before.Data;
    const auto& after = round.After.Data;
    const auto& filter = session.Filter;

    const size_t stride = ScanDataTypeSize(filter.DataType);
    const size_t count = (std::min)(before.size(), after.size());

    std::vector<TypedMatch> matches;

    for (size_t i = 0; i + stride <= count; i += stride)
    {
        uint64_t vBefore = ExtractRaw(before, i, filter.DataType);
        uint64_t vAfter = ExtractRaw(after, i, filter.DataType);

        if (MatchesPair(vBefore, vAfter, filter))
        {
            TypedMatch m;
            m.Offset = i;
            m.ValueBefore = vBefore;
            m.ValueAfter = vAfter;
            m.DataType = filter.DataType;
            matches.push_back(m);
        }
    }

    g_pState->Infrastructure->MemoryScanner->SetRoundTypedDiffs(std::move(matches));
    g_pState->Infrastructure->MemoryScanner->CompleteRound();
}

void System_MemoryScanner::ComputeExactMatchFirstPass(const ScanFilter& f)
{
    const auto& session = g_pState->Infrastructure->MemoryScanner->GetSession();
    if (session.Rounds.empty()) return;

    const auto& snap = session.Rounds.back().Before.Data; // snapshot único
    const size_t stride = ScanDataTypeSize(f.DataType);

    std::vector<TypedMatch> matches;
    for (size_t i = 0; i + stride <= snap.size(); i += stride)
    {
        uint64_t val = ExtractRaw(snap, i, f.DataType);
        if (MatchesPair(val, val, f))  // before == after en primer pass
        {
            TypedMatch m;
            m.Offset = i;
            m.ValueBefore = val;
            m.ValueAfter = val;
            m.DataType = f.DataType;
            matches.push_back(m);
        }
    }

    g_pState->Infrastructure->MemoryScanner->SetRoundTypedDiffs(std::move(matches));
    g_pState->Infrastructure->MemoryScanner->CompleteRound();
}

bool System_MemoryScanner::TryReadMemory(uintptr_t base, size_t size, uint8_t* outBuffer)
{
    __try { std::memcpy(outBuffer, reinterpret_cast<void*>(base), size); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

std::vector<uint8_t> System_MemoryScanner::ReadMemory(uintptr_t base, size_t size)
{
    std::vector<uint8_t> buffer(size, 0);
    TryReadMemory(base, size, buffer.data());
    return buffer;
}