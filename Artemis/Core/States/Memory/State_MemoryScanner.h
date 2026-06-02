#pragma once

#include "Core/Types/Memory/MemoryScannerTypes.h"

#include <string>
#include <vector>

class State_MemoryScanner
{
public:
    const ScanSession& GetSession() const;

    int  GetDelayMs() const;
    void SetDelayMs(int ms);

    const ScanFilter& GetFilter() const;
    void SetFilter(const ScanFilter& filter);

    void SetRegion(const std::string& name, uintptr_t base, size_t size);
    void SetWaitingForKey(bool waiting);

    void BeginRound();
    void BeginUnchangedRound();
    void SetRoundBefore(ScanSnapshot snap);
    void SetRoundAfter(ScanSnapshot snap);
    void SetRoundDiffs(std::vector<ByteDiff> diffs);
    void SetRoundTypedDiffs(std::vector<TypedMatch> matches);
    void CompleteRound();
    void ComputeFinalDiffs();
    void Reset();

    bool IsWaitingForKey() const;
    bool IsScanning()   const;
    void SetScanning(bool scanning);

private:
    ScanSession m_Session;
    int  m_DelayMs = 500;
    bool m_WaitingForKey = false;
    bool m_Scanning = false;
};