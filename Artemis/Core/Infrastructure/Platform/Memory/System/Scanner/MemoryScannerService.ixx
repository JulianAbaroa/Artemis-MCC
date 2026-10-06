export module Platform.Memory.System:Scanner;

import Service.Logs.System;
import Platform.Memory.Type;
import Platform.Memory.State;
import std;

export namespace Platform::Memory::System
{
    // Scans a memory region on a worker thread and stores the matches in the MemoryScannerStore.
    // note: Each Trigger call runs one round. Repeating it narrows the result. Calls are ignored while scanning.
    class MemoryScannerService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using DataType = Platform::Memory::Type::DataType;
        using Filter = Platform::Memory::Type::Filter;
        using Snapshot = Platform::Memory::Type::Snapshot;

        using MemoryScannerStore = Platform::Memory::State::MemoryScannerStore;

    public:
        MemoryScannerService(LogsService& logsService,
            MemoryScannerStore& memoryScannerStore) : m_LogsService(logsService),
            m_MemoryScannerStore(memoryScannerStore) {}
        ~MemoryScannerService()
        {
            if (m_WorkerThread.joinable()) m_WorkerThread.join();
        }

        // Sets the region to scan.
        auto SetRegion(const std::string& name, std::uintptr_t base, std::size_t size) -> void;

        // Differential scans. Take a snapshot, wait delayMs and take another.
        // param delayMs: Wait between the two snapshots.
        auto TriggerScan(int delayMs) -> void;
        auto TriggerUnchangedScan(int delayMs) -> void;
        auto TriggerIncreasedScan(DataType type, int delayMs) -> void;
        auto TriggerDecreasedScan(DataType type, int delayMs) -> void;
        auto TriggerIncreasedByScan(DataType type, std::uint64_t delta, int delayMs) -> void;
        auto TriggerDecreasedByScan(DataType type, std::uint64_t delta, int delayMs) -> void;

        // Value scans. The Float versions take the value as float instead of raw bits.
        // note: The exact scan takes a single snapshot, so it has no delay.
        auto TriggerExactScan(DataType type, std::uint64_t value) -> void;
        auto TriggerExactScanFloat(float value) -> void;

        auto TriggerInRangeScan(DataType type, std::uint64_t lo, std::uint64_t hi, int delayMs) -> void;
        auto TriggerInRangeScanFloat(float lo, float hi, int delayMs) -> void;

        auto TriggerBitMaskScan(std::uint32_t mask, std::uint32_t pattern, int delayMs) -> void;

        // Waits for the worker thread and clears the session.
        auto Reset() -> void;

    private:
        LogsService& m_LogsService;
        MemoryScannerStore& m_MemoryScannerStore;

        std::thread m_WorkerThread{};

        // Starts a differential round with the filter. Does nothing if already scanning.
        auto StartScan(const Filter& filter, int delayMs, bool isUnchangedRound = false) -> void;
        auto RunDifferentialScan(int delayMs) -> void;
        auto CaptureSnapshot(bool isBefore) -> void;

        auto ComputeRoundDiff() -> void;
        auto ComputeTypedRoundDiff() -> void;
        auto ComputeExactMatchFirstPass(const Filter& filter) -> void;

        auto MatchesPair(std::uint64_t before, std::uint64_t after, const Filter& filter) const -> bool;

        // return: The value at offset as raw bits. Floats are returned as their bit pattern.
        static auto ExtractRaw(const std::vector<std::uint8_t>& buffer, std::size_t offset, DataType type) -> std::uint64_t;

        auto TryReadMemory(std::uintptr_t base, std::size_t size, std::uint8_t* outBuffer) -> bool;
        auto ReadMemory(std::uintptr_t base, std::size_t size) -> std::vector<std::uint8_t>;
    };
}