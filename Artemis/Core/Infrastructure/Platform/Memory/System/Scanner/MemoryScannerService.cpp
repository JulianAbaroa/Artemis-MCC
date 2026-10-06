module;

#include <windows.h>

module Platform.Memory.System;
import :Scanner;

namespace
{
    using Mode = Platform::Memory::Type::Mode;
    using ByteDiff = Platform::Memory::Type::ByteDiff;
    using TypedMatch = Platform::Memory::Type::TypedMatch;

    using std::chrono::milliseconds;
}

namespace Platform::Memory::System
{
    auto MemoryScannerService::SetRegion(const std::string& name, std::uintptr_t base, std::size_t size) -> void
    {
        m_MemoryScannerStore.SetRegion(name, base, size);
    }

    auto MemoryScannerService::TriggerScan(int delayMs) -> void
    {
        this->StartScan({ .Mode = Mode::Changed, .DataType = DataType::Bytes }, delayMs);
    }

    auto MemoryScannerService::TriggerUnchangedScan(int delayMs) -> void
    {
        this->StartScan({ .Mode = Mode::Unchanged, .DataType = DataType::Bytes }, delayMs, true);
    }

    auto MemoryScannerService::TriggerIncreasedScan(DataType type, int delayMs) -> void
    {
        this->StartScan({ .Mode = Mode::Increased, .DataType = type }, delayMs);
    }

    auto MemoryScannerService::TriggerDecreasedScan(DataType type, int delayMs) -> void
    {
        this->StartScan({ .Mode = Mode::Decreased, .DataType = type }, delayMs);
    }

    auto MemoryScannerService::TriggerIncreasedByScan(DataType type, std::uint64_t delta, int delayMs) -> void
    {
        this->StartScan({ .Mode = Mode::IncreasedBy, .DataType = type, .ValueA = delta }, delayMs);
    }

    auto MemoryScannerService::TriggerDecreasedByScan(DataType type, std::uint64_t delta, int delayMs) -> void
    {
        this->StartScan({ .Mode = Mode::DecreasedBy, .DataType = type, .ValueA = delta }, delayMs);
    }

    auto MemoryScannerService::TriggerExactScan(DataType type, std::uint64_t value) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        Filter filter{ .Mode = Mode::ExactValue, .DataType = type, .ValueA = value };
        m_MemoryScannerStore.SetFilter(filter);
        m_MemoryScannerStore.BeginRound();

        // First pass: a single snapshot is enough, there is no "after".
        this->CaptureSnapshot(true);

        if (m_WorkerThread.joinable()) m_WorkerThread.join();

        m_WorkerThread = std::thread([this, filter]() {
            this->ComputeExactMatchFirstPass(filter);
            m_MemoryScannerStore.ComputeFinalDiffs();
            m_MemoryScannerStore.SetScanning(false);
            });
    }

    auto MemoryScannerService::TriggerExactScanFloat(float value) -> void
    {
        this->TriggerExactScan(DataType::Float32, std::bit_cast<std::uint32_t>(value));
    }

    auto MemoryScannerService::TriggerInRangeScan(DataType type, std::uint64_t lo, std::uint64_t hi, int delayMs) -> void
    {
        this->StartScan({ .Mode = Mode::InRange, .DataType = type, .ValueA = lo, .ValueB = hi }, delayMs);
    }

    auto MemoryScannerService::TriggerInRangeScanFloat(float lo, float hi, int delayMs) -> void
    {
        this->TriggerInRangeScan(DataType::Float32, std::bit_cast<std::uint32_t>(lo),
            std::bit_cast<std::uint32_t>(hi), delayMs);
    }

    auto MemoryScannerService::TriggerBitMaskScan(std::uint32_t mask, std::uint32_t pattern, int delayMs) -> void
    {
        this->StartScan({ .Mode = Mode::BitMask, .DataType = DataType::UInt32,
            .BitMaskPattern = mask, .BitMaskValue = pattern }, delayMs);
    }

    auto MemoryScannerService::Reset() -> void
    {
        if (m_WorkerThread.joinable()) m_WorkerThread.join();

        m_MemoryScannerStore.SetScanning(false);
        m_MemoryScannerStore.Reset();
    }

    auto MemoryScannerService::StartScan(const Filter& filter, int delayMs, bool isUnchangedRound) -> void
    {
        if (m_MemoryScannerStore.IsScanning()) return;
        m_MemoryScannerStore.SetScanning(true);

        m_MemoryScannerStore.SetFilter(filter);

        if (isUnchangedRound) m_MemoryScannerStore.BeginUnchangedRound();
        else m_MemoryScannerStore.BeginRound();

        this->RunDifferentialScan(delayMs);
    }

    auto MemoryScannerService::RunDifferentialScan(int delayMs) -> void
    {
        this->CaptureSnapshot(true);

        if (m_WorkerThread.joinable()) m_WorkerThread.join();

        m_WorkerThread = std::thread([this, delayMs]() {
            std::this_thread::sleep_for(milliseconds{ delayMs });

            this->CaptureSnapshot(false);

            if (m_MemoryScannerStore.GetFilter().DataType == DataType::Bytes) this->ComputeRoundDiff();
            else this->ComputeTypedRoundDiff();

            m_MemoryScannerStore.ComputeFinalDiffs();
            m_MemoryScannerStore.SetScanning(false);
            });
    }

    auto MemoryScannerService::CaptureSnapshot(bool isBefore) -> void
    {
        const auto& region = m_MemoryScannerStore.GetSession().Region;
        if (region.BaseAddress == 0 || region.Size == 0) return;

        Snapshot snapshot{};
        snapshot.Data = this->ReadMemory(region.BaseAddress, region.Size);
        snapshot.Label = isBefore ? "Before" : "After";
        snapshot.BaseAddress = region.BaseAddress;

        if (isBefore) m_MemoryScannerStore.SetRoundBefore(std::move(snapshot));
        else m_MemoryScannerStore.SetRoundAfter(std::move(snapshot));
    }

    auto MemoryScannerService::ComputeRoundDiff() -> void
    {
        const auto& session = m_MemoryScannerStore.GetSession();
        if (session.Rounds.empty()) return;

        const auto& round = session.Rounds.back();
        const auto& before = round.Before.Data;
        const auto& after = round.After.Data;
        const std::size_t count = (std::min)(before.size(), after.size());

        std::vector<ByteDiff> diffs{};

        for (std::size_t i = 0; i < count; ++i)
        {
            if (before[i] != after[i]) diffs.push_back({ i, before[i], after[i] });
        }

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

        const std::size_t stride = Platform::Memory::Type::ScanDataTypeSize(filter.DataType);
        const std::size_t count = (std::min)(before.size(), after.size());

        std::vector<TypedMatch> matches{};

        for (std::size_t i = 0; i + stride <= count; i += stride)
        {
            const std::uint64_t valueBefore = ExtractRaw(before, i, filter.DataType);
            const std::uint64_t valueAfter = ExtractRaw(after, i, filter.DataType);

            if (this->MatchesPair(valueBefore, valueAfter, filter))
            {
                matches.push_back({ i, valueBefore, valueAfter, filter.DataType });
            }
        }

        m_MemoryScannerStore.SetRoundTypedDiffs(std::move(matches));
        m_MemoryScannerStore.CompleteRound();
    }

    auto MemoryScannerService::ComputeExactMatchFirstPass(const Filter& filter) -> void
    {
        const auto& session = m_MemoryScannerStore.GetSession();
        if (session.Rounds.empty()) return;

        const auto& snapshot = session.Rounds.back().Before.Data;
        const std::size_t stride = Platform::Memory::Type::ScanDataTypeSize(filter.DataType);

        std::vector<TypedMatch> matches{};

        for (std::size_t i = 0; i + stride <= snapshot.size(); i += stride)
        {
            const std::uint64_t value = ExtractRaw(snapshot, i, filter.DataType);

            // Single snapshot: before and after are the same value.
            if (this->MatchesPair(value, value, filter))
            {
                matches.push_back({ i, value, value, filter.DataType });
            }
        }

        m_MemoryScannerStore.SetRoundTypedDiffs(std::move(matches));
        m_MemoryScannerStore.CompleteRound();
    }

    auto MemoryScannerService::MatchesPair(std::uint64_t before, std::uint64_t after, const Filter& filter) const -> bool
    {
        switch (filter.Mode)
        {
        case Mode::Changed:     return before != after;
        case Mode::Unchanged:   return before == after;
        case Mode::Increased:   return after > before;
        case Mode::Decreased:   return after < before;
        case Mode::IncreasedBy: return (after - before) == filter.ValueA;
        case Mode::DecreasedBy: return (before - after) == filter.ValueA;
        case Mode::ExactValue:  return after == filter.ValueA;
        case Mode::InRange:     return after >= filter.ValueA && after <= filter.ValueB;

        case Mode::BitMask:
            return (static_cast<std::uint32_t>(after) & filter.BitMaskPattern) == filter.BitMaskValue;

        default:
            return false;
        }
    }

    auto MemoryScannerService::ExtractRaw(const std::vector<std::uint8_t>& buffer, std::size_t offset, DataType type) -> std::uint64_t
    {
        const std::size_t size = Platform::Memory::Type::ScanDataTypeSize(type);

        switch (type)
        {
        case DataType::Int8:
            return static_cast<std::uint64_t>(std::bit_cast<std::int8_t>(buffer[offset]));

        case DataType::Int16:
        {
            std::int16_t value{};
            std::memcpy(&value, buffer.data() + offset, size);
            return static_cast<std::uint64_t>(value);
        }

        case DataType::Int32:
        {
            std::int32_t value{};
            std::memcpy(&value, buffer.data() + offset, size);
            return static_cast<std::uint64_t>(value);
        }

        case DataType::UInt16:
        {
            std::uint16_t value{};
            std::memcpy(&value, buffer.data() + offset, size);
            return value;
        }

        case DataType::UInt32:
        case DataType::Float32:
        {
            std::uint32_t value{};
            std::memcpy(&value, buffer.data() + offset, size);
            return value;
        }

        default:
            return buffer[offset];
        }
    }

    auto MemoryScannerService::TryReadMemory(std::uintptr_t base, std::size_t size, std::uint8_t* outBuffer) -> bool
    {
        __try
        {
            std::memcpy(outBuffer, reinterpret_cast<void*>(base), size);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    auto MemoryScannerService::ReadMemory(std::uintptr_t base, std::size_t size) -> std::vector<std::uint8_t>
    {
        std::vector<std::uint8_t> buffer(size, 0);

        this->TryReadMemory(base, size, buffer.data());

        return buffer;
    }
}