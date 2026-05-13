#pragma once

// Types.
#include "Core/Types/Infrastructure/MemoryScannerTypes.h"

#include <string>
#include <vector>

class State_MemoryScanner
{
public:
    void SetRegion(const std::string& name, uintptr_t base, size_t size);
    void SetFilter(const ScanFilter& filter);
    void SetDelayMs(int ms);
    void SetWaitingForKey(bool waiting);
    void SetBoundKey(int key);

    void BeginRound();
    void BeginUnchangedRound();
    void SetRoundBefore(ScanSnapshot snap);
    void SetRoundAfter(ScanSnapshot snap);
    void SetRoundDiffs(std::vector<ByteDiff> diffs);
    void SetRoundTypedDiffs(std::vector<TypedMatch> matches);
    void CompleteRound();
    void ComputeFinalDiffs();
    void Reset();

    const ScanSession& GetSession() const;
    const ScanFilter& GetFilter()  const;
    int  GetDelayMs()   const;
    int  GetBoundKey()  const;
    bool IsWaitingForKey() const;
    bool IsScanning()   const;
    void SetScanning(bool scanning);

private:
    ScanSession m_Session;
    int  m_DelayMs = 500;
    int  m_BoundKey = VK_F5;
    bool m_WaitingForKey = false;
    bool m_Scanning = false;
};