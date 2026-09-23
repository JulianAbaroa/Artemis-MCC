export module Platform.Memory.System:Scanner;

import Service.Logs.System;
import Platform.Memory.Type;
import Platform.Memory.State;
import std;

export namespace Platform::Memory::System
{
    class MemoryScannerService
    {
    private:
        using DataType = Platform::Memory::Type::DataType;
        using Filter = Platform::Memory::Type::Filter;
        using Snapshot = Platform::Memory::Type::Snapshot;

        using LogsService = Service::Logs::System::LogsService;
        using MemoryScannerStore = Platform::Memory::State::MemoryScannerStore;

    public:
        MemoryScannerService(LogsService& logsService, 
            MemoryScannerStore& memoryScannerStore) : m_LogsService(logsService),
            m_MemoryScannerStore(memoryScannerStore) {}
        ~MemoryScannerService()
        {
            if (m_WorkerThread.joinable()) m_WorkerThread.join();
        }

        auto SetRegion(const std::string& name, std::uintptr_t base, std::size_t size) -> void;

        auto TriggerScan(int delayMs) -> void;
        auto TriggerUnchangedScan(int delayMs) -> void;
        auto TriggerIncreasedScan(DataType type, int delayMs) -> void;
        auto TriggerDecreasedScan(DataType type, int delayMs) -> void;
        auto TriggerIncreasedByScan(DataType type, std::uint64_t delta, int delayMs) -> void;
        auto TriggerDecreasedByScan(DataType type, std::uint64_t delta, int delayMs) -> void;

        auto TriggerExactScan(DataType type, std::uint64_t value, int delayMs) -> void;
        auto TriggerExactScanFloat(float value, int delayMs) -> void;

        auto TriggerInRangeScan(DataType type, std::uint64_t lo, std::uint64_t hi, int delayMs) -> void;
        auto TriggerInRangeScanFloat(float lo, float hi, int delayMs) -> void;

        auto TriggerBitMaskScan(std::uint32_t mask, std::uint32_t pattern, int delayMs) -> void;
        auto TriggerStabilizedScan(DataType type, int rounds, int delayMs) -> void;

        auto Reset() -> void;

    private:
        LogsService& m_LogsService;
        MemoryScannerStore& m_MemoryScannerStore;

        std::thread m_WorkerThread{};

        auto RunDifferentialScan(int delayMs) -> void;
        auto CaptureSnapshot(bool isBefore) -> void;

        auto ComputeRoundDiff() -> void;

        auto ComputeTypedRoundDiff() -> void;

        template<typename T>
        auto ReadAt(const std::vector<std::uint8_t>& buf, std::size_t offset) const -> T;

        static auto ExtractRaw(const std::vector<std::uint8_t>& buf, std::size_t offset, DataType type) -> std::uint64_t;

        auto MatchesPair(std::uint64_t before, std::uint64_t after, const Filter& f) const -> bool;

        auto ComputeExactMatchFirstPass(const Filter& f) -> void;

        auto TryReadMemory(std::uintptr_t base, std::size_t size, std::uint8_t* outBuffer) -> bool;
        auto ReadMemory(std::uintptr_t base, std::size_t size) -> std::vector<std::uint8_t>;
    };
}