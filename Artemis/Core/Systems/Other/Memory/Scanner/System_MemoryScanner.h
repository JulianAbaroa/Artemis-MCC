#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <thread>

struct ScanFilter;
enum class ScanDataType : uint8_t;

class State_MemoryScanner;
class System_Logs;

struct Sys_MemoryScanner_Deps
{
    State_MemoryScanner& State_MemoryScanner;
    System_Logs& System_Logs;
};

class System_MemoryScanner
{
public:
    System_MemoryScanner(Sys_MemoryScanner_Deps deps) : m_Deps(deps) {}
    ~System_MemoryScanner()
    {
        if (m_WorkerThread.joinable()) m_WorkerThread.join();
    }

    void SetRegion(const std::string& name, uintptr_t base, size_t size);

    void TriggerScan(int delayMs);
    void TriggerUnchangedScan(int delayMs);
    void TriggerIncreasedScan(ScanDataType type, int delayMs);
    void TriggerDecreasedScan(ScanDataType type, int delayMs);
    void TriggerIncreasedByScan(ScanDataType type, uint64_t delta, int delayMs);
    void TriggerDecreasedByScan(ScanDataType type, uint64_t delta, int delayMs);

    void TriggerExactScan(ScanDataType type, uint64_t value, int delayMs);
    void TriggerExactScanFloat(float value, int delayMs);

    void TriggerInRangeScan(ScanDataType type, uint64_t lo, uint64_t hi, int delayMs);
    void TriggerInRangeScanFloat(float lo, float hi, int delayMs);

    void TriggerBitMaskScan(uint32_t mask, uint32_t pattern, int delayMs);
    void TriggerStabilizedScan(ScanDataType type, int rounds, int delayMs);

    void Reset();

private:
    Sys_MemoryScanner_Deps m_Deps;

    void RunDifferentialScan(int delayMs);   
    void CaptureSnapshot(bool isBefore);

    void ComputeRoundDiff();

    void ComputeTypedRoundDiff();

    template<typename T>
    T ReadAt(const std::vector<uint8_t>& buf, size_t offset) const;

    bool MatchesPair(uint64_t before, uint64_t after, const ScanFilter& f) const;

    void ComputeExactMatchFirstPass(const ScanFilter& f);

    bool TryReadMemory(uintptr_t base, size_t size, uint8_t* outBuffer);
    std::vector<uint8_t> ReadMemory(uintptr_t base, size_t size);

    std::thread m_WorkerThread;
};