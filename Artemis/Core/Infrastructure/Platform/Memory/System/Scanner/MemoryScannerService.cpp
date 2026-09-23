module;

#include <windows.h>

module Platform.Memory.System;
import :Scanner;

namespace
{
    using Mode = Platform::Memory::Type::Mode;
    using ByteDiff = Platform::Memory::Type::ByteDiff;
    using TypedMatch = Platform::Memory::Type::TypedMatch;
    using Milliseconds = std::chrono::milliseconds;
}

namespace Platform::Memory::System
{
	auto MemoryScannerService::MatchesPair(std::uint64_t before, std::uint64_t after, const Filter& f) const -> bool
    {
        switch (f.Mode)
        {
        case Mode::Changed:       return before != after;
        case Mode::Unchanged:     return before == after;
        case Mode::Increased:     return after > before;
        case Mode::Decreased:     return after < before;
        case Mode::IncreasedBy:   return (after - before) == f.ValueA;
        case Mode::DecreasedBy:   return (before - after) == f.ValueA;

        case Mode::ExactValue:
            // In refinement: the current value must be the expected value
            return after == f.ValueA;

        case Mode::InRange:
            return after >= f.ValueA && after <= f.ValueB;

        case Mode::BitMask:
            return (static_cast<std::uint32_t>(after) & f.BitMaskPattern) == f.BitMaskValue;

        case Mode::Stabilized:
            // It is handled at the round level in ComputeFinalDiffs
            return before == after;

        default: return false;
        }
    }

    // Base differential scan

    auto MemoryScannerService::RunDifferentialScan(int delayMs) -> void
    {
        CaptureSnapshot(true);

        if (m_WorkerThread.joinable()) m_WorkerThread.join();

        m_WorkerThread = std::thread([this, delayMs]() {
            std::this_thread::sleep_for(Milliseconds(delayMs));

            this->CaptureSnapshot(false);

            const auto& filter = m_MemoryScannerStore.GetFilter();
            if (filter.DataType == DataType::Bytes)
                this->ComputeRoundDiff();
            else
                this->ComputeTypedRoundDiff();

            m_MemoryScannerStore.ComputeFinalDiffs();
            m_MemoryScannerStore.SetScanning(false);
            });
    }

    // Public API

    auto MemoryScannerService::SetRegion(const std::string& name, std::uintptr_t base, std::size_t size) -> void
    {
        m_MemoryScannerStore.SetRegion(name, base, size);
    }

    auto MemoryScannerService::TriggerScan(int delayMs) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter f; f.Mode = Mode::Changed; f.DataType = DataType::Bytes;
        m_MemoryScannerStore.SetFilter(f);
        m_MemoryScannerStore.BeginRound();
        RunDifferentialScan(delayMs);
    }

    auto MemoryScannerService::TriggerUnchangedScan(int delayMs) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter f; f.Mode = Mode::Unchanged; f.DataType = DataType::Bytes;
        m_MemoryScannerStore.SetFilter(f);
        m_MemoryScannerStore.BeginUnchangedRound();
        RunDifferentialScan(delayMs);
    }

    auto MemoryScannerService::TriggerIncreasedScan(DataType type, int delayMs) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter f; f.Mode = Mode::Increased; f.DataType = type;
        m_MemoryScannerStore.SetFilter(f);
        m_MemoryScannerStore.BeginRound();
        RunDifferentialScan(delayMs);
    }

    auto MemoryScannerService::TriggerDecreasedScan(DataType type, int delayMs) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter f; f.Mode = Mode::Decreased; f.DataType = type;
        m_MemoryScannerStore.SetFilter(f);
        m_MemoryScannerStore.BeginRound();
        RunDifferentialScan(delayMs);
    }

    auto MemoryScannerService::TriggerIncreasedByScan(DataType type, std::uint64_t delta, int delayMs) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter f; f.Mode = Mode::IncreasedBy; f.DataType = type; f.ValueA = delta;
        m_MemoryScannerStore.SetFilter(f);
        m_MemoryScannerStore.BeginRound();
        RunDifferentialScan(delayMs);
    }

    auto MemoryScannerService::TriggerDecreasedByScan(DataType type, std::uint64_t delta, int delayMs) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter f; f.Mode = Mode::DecreasedBy; f.DataType = type; f.ValueA = delta;
        m_MemoryScannerStore.SetFilter(f);
        m_MemoryScannerStore.BeginRound();
        RunDifferentialScan(delayMs);
    }

    // Exact Value, first pass: no need for Before, just the current snapshot
    auto MemoryScannerService::TriggerExactScan(DataType type, std::uint64_t value, int delayMs) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter f; f.Mode = Mode::ExactValue; f.DataType = type; f.ValueA = value;
        m_MemoryScannerStore.SetFilter(f);
        m_MemoryScannerStore.BeginRound();

        CaptureSnapshot(true);

        if (m_WorkerThread.joinable()) m_WorkerThread.join();
        m_WorkerThread = std::thread([this, f]() {
            std::this_thread::yield();

            this->ComputeExactMatchFirstPass(f);
            m_MemoryScannerStore.ComputeFinalDiffs();
            m_MemoryScannerStore.SetScanning(false);
            });
    }

    auto MemoryScannerService::TriggerExactScanFloat(float value, int delayMs) -> void
    {
        std::uint32_t raw; std::memcpy(&raw, &value, 4);
        TriggerExactScan(DataType::Float32, raw, delayMs);
    }

    auto MemoryScannerService::TriggerInRangeScan(DataType type, std::uint64_t lo, std::uint64_t hi, int delayMs) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter f; f.Mode = Mode::InRange; f.DataType = type; f.ValueA = lo; f.ValueB = hi;
        m_MemoryScannerStore.SetFilter(f);
        m_MemoryScannerStore.BeginRound();
        RunDifferentialScan(delayMs);
    }

    auto MemoryScannerService::TriggerInRangeScanFloat(float lo, float hi, int delayMs) -> void
    {
        std::uint32_t rlo, rhi;
        std::memcpy(&rlo, &lo, 4);
        std::memcpy(&rhi, &hi, 4);
        TriggerInRangeScan(DataType::Float32, rlo, rhi, delayMs);
    }

    auto MemoryScannerService::TriggerBitMaskScan(std::uint32_t mask, std::uint32_t pattern, int delayMs) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter f;
        f.Mode = Mode::BitMask;
        f.DataType = DataType::UInt32;
        f.BitMaskPattern = mask;
        f.BitMaskValue = pattern;
        m_MemoryScannerStore.SetFilter(f);
        m_MemoryScannerStore.BeginRound();
        RunDifferentialScan(delayMs);
    }

    auto MemoryScannerService::TriggerStabilizedScan(DataType type, int rounds, int delayMs) -> void
    {
        // Stabilized: Triggers N rounds of typed "Unchanged" and then ComputeFinalDiffs filters them.
        // Each call to TriggerStabilizedScan adds one round, the caller calls it N times.
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter f; f.Mode = Mode::Stabilized; f.DataType = type; f.StabilizeRounds = rounds;
        m_MemoryScannerStore.SetFilter(f);
        m_MemoryScannerStore.BeginRound();
        RunDifferentialScan(delayMs);
    }

    auto MemoryScannerService::Reset() -> void
    {
        if (m_WorkerThread.joinable()) m_WorkerThread.join();
        m_MemoryScannerStore.SetScanning(false);
        m_MemoryScannerStore.Reset();
    }

    // Diff logic

    auto MemoryScannerService::CaptureSnapshot(bool isBefore) -> void
    {
        const auto& region = m_MemoryScannerStore.GetSession().Region;
        if (region.BaseAddress == 0 || region.Size == 0) return;

        Snapshot snap;
        snap.Data = ReadMemory(region.BaseAddress, region.Size);
        snap.Label = isBefore ? "Before" : "After";
        snap.BaseAddress = region.BaseAddress;

        if (isBefore) m_MemoryScannerStore.SetRoundBefore(std::move(snap));
        else m_MemoryScannerStore.SetRoundAfter(std::move(snap));
    }

    auto MemoryScannerService::ComputeRoundDiff() -> void
    {
        const auto& session = m_MemoryScannerStore.GetSession();
        if (session.Rounds.empty()) return;

        const auto& round = session.Rounds.back();
        const auto& before = round.Before.Data;
        const auto& after = round.After.Data;
        std::size_t count = (std::min)(before.size(), after.size());

        std::vector<ByteDiff> diffs;
        for (std::size_t i = 0; i < count; ++i)
            if (before[i] != after[i])
                diffs.push_back({ i, before[i], after[i] });

        m_MemoryScannerStore.SetRoundDiffs(std::move(diffs));
        m_MemoryScannerStore.CompleteRound();
    }

    auto MemoryScannerService::ComputeTypedRoundDiff() -> void
    {
        const auto& session = m_MemoryScannerStore.GetSession();
        if (session.Rounds.empty()) return;

        const auto& round = session.Rounds.back();
        const auto& before = round.Before.Data;
        const auto& after = round.After.Data;
        const auto& filter = session.Filter;

        const std::size_t stride = ScanDataTypeSize(filter.DataType);
        const std::size_t count = (std::min)(before.size(), after.size());

        std::vector<TypedMatch> matches;

        for (std::size_t i = 0; i + stride <= count; i += stride)
        {
            std::uint64_t vBefore = ExtractRaw(before, i, filter.DataType);
            std::uint64_t vAfter = ExtractRaw(after, i, filter.DataType);

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

        m_MemoryScannerStore.SetRoundTypedDiffs(std::move(matches));
        m_MemoryScannerStore.CompleteRound();
    }

    auto MemoryScannerService::ComputeExactMatchFirstPass(const Filter& f) -> void
    {
        const auto& session = m_MemoryScannerStore.GetSession();
        if (session.Rounds.empty()) return;

        const auto& snap = session.Rounds.back().Before.Data; // snapshot único
        const std::size_t stride = ScanDataTypeSize(f.DataType);

        std::vector<TypedMatch> matches;
        for (std::size_t i = 0; i + stride <= snap.size(); i += stride)
        {
            std::uint64_t val = ExtractRaw(snap, i, f.DataType);
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

        m_MemoryScannerStore.SetRoundTypedDiffs(std::move(matches));
        m_MemoryScannerStore.CompleteRound();
    }

    auto MemoryScannerService::TryReadMemory(std::uintptr_t base, std::size_t size, std::uint8_t* outBuffer) -> bool
    {
        __try {
            std::memcpy(outBuffer, reinterpret_cast<void*>(base), size);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    auto MemoryScannerService::ReadMemory(std::uintptr_t base, std::size_t size) -> std::vector<std::uint8_t>
    {
        std::vector<std::uint8_t> buffer(size, 0);
        TryReadMemory(base, size, buffer.data());
        return buffer;
    }

    // Helpers.
    template<typename T>
    auto MemoryScannerService::ReadAt(const std::vector<std::uint8_t>& buf, std::size_t offset) const -> T
    {
        T val{};
        std::memcpy(&val, buf.data() + offset, sizeof(T));
        return val;
    }

    // Extract the raw value as uint64_t from the buffer at the given offset
    auto MemoryScannerService::ExtractRaw(const std::vector<uint8_t>& buf, std::size_t offset, DataType type) -> std::uint64_t
    {
        switch (type)
        {
        case DataType::Int8: { std::int8_t  v; std::memcpy(&v, buf.data() + offset, 1); return static_cast<std::uint64_t>(v); }
        case DataType::UInt8: { std::uint8_t v; std::memcpy(&v, buf.data() + offset, 1); return v; }
        case DataType::Int16: { std::int16_t  v; std::memcpy(&v, buf.data() + offset, 2); return static_cast<std::uint64_t>(v); }
        case DataType::UInt16: { std::uint16_t v; std::memcpy(&v, buf.data() + offset, 2); return v; }
        case DataType::Int32: { std::int32_t  v; std::memcpy(&v, buf.data() + offset, 4); return static_cast<std::uint64_t>(v); }
        case DataType::UInt32: { std::uint32_t v; std::memcpy(&v, buf.data() + offset, 4); return v; }
        case DataType::Float32: { std::uint32_t v; std::memcpy(&v, buf.data() + offset, 4); return v; }
        default: return buf[offset];
        }
    }
}