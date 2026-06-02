#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <atomic>
#include <thread>

struct ScanFilter;
enum class ScanDataType : uint8_t;

class State_MemoryScanner;
class System_Logs;

struct System_MemoryScanner_Dependencies
{
    State_MemoryScanner& State_MemoryScanner;
    System_Logs& System_Logs;
};

class System_MemoryScanner
{
public:
    System_MemoryScanner(System_MemoryScanner_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~System_MemoryScanner()
    {
        if (m_WorkerThread.joinable()) m_WorkerThread.join();
    }

    void SetRegion(const std::string& name, uintptr_t base, size_t size);

    // Byte-by-byte mode.
    void TriggerScan(int delayMs);
    void TriggerUnchangedScan(int delayMs);

    // Typed modes.
    void TriggerIncreasedScan(ScanDataType type, int delayMs);
    void TriggerDecreasedScan(ScanDataType type, int delayMs);
    void TriggerIncreasedByScan(ScanDataType type, uint64_t delta, int delayMs);
    void TriggerDecreasedByScan(ScanDataType type, uint64_t delta, int delayMs);

    // Exact Value: searches for offsets that have exactly this value. 
    // First call: scans the current snapshot. 
    // Subsequent calls: refine the set of matches.
    void TriggerExactScan(ScanDataType type, uint64_t value, int delayMs);
    void TriggerExactScanFloat(float value, int delayMs);

    void TriggerInRangeScan(ScanDataType type, uint64_t lo, uint64_t hi, int delayMs);
    void TriggerInRangeScanFloat(float lo, float hi, int delayMs);

    void TriggerBitMaskScan(uint32_t mask, uint32_t pattern, int delayMs);
    void TriggerStabilizedScan(ScanDataType type, int rounds, int delayMs);

    void Reset();

private:
    System_MemoryScanner_Dependencies m_Deps;

    // Capture Before/After and call the diff
    void RunDifferentialScan(int delayMs);   
    void CaptureSnapshot(bool isBefore);

    // Diff byte by byte (Bytes mode)
    void ComputeRoundDiff();

    // Typed Diff: delegates according to the active filter
    void ComputeTypedRoundDiff();

    // Aligned interpretation of a buffer
    template<typename T>
    T ReadAt(const std::vector<uint8_t>& buf, size_t offset) const;

    // Typed comparison of a pair (before, after) according to the filter
    bool MatchesPair(uint64_t before, uint64_t after, const ScanFilter& f) const;

    // For ExactValue: first scan (no Before)
    void ComputeExactMatchFirstPass(const ScanFilter& f);

    bool TryReadMemory(uintptr_t base, size_t size, uint8_t* outBuffer);
    std::vector<uint8_t> ReadMemory(uintptr_t base, size_t size);

    std::thread m_WorkerThread;
};