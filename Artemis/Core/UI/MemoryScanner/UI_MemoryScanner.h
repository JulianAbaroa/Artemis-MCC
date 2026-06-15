#pragma once

#include "Core/UI/Launcher/UI_Tab.h"

#include <vector>
#include <string>

struct ScanSession;
enum class ScanDataType : uint8_t;
enum class ScanMode : uint8_t;

class State_MemoryScanner;
class System_MemoryScanner;

struct UI_MemoryScanner_Deps
{
    State_MemoryScanner& State_MemoryScanner;
    System_MemoryScanner& System_MemoryScanner;
};

class UI_MemoryScanner : public UI_Tab
{
public:
    UI_MemoryScanner(UI_MemoryScanner_Deps dependencies) :
        m_Deps(dependencies) {};
    ~UI_MemoryScanner() = default;
    void Draw();

private:
    UI_MemoryScanner_Deps m_Deps;

    void DrawTopBar();
    void DrawRegionInputs();
    void DrawKnownSizesCombo() const;
    void DrawTriggerResetButtons();
    void DrawScanStatus();
    void DrawRoundHistoryTooltip(const ScanSession& session, bool isTyped);
    void DispatchScan();

    void DrawModeConfig();
    void DrawModeCombo();
    void DrawDataTypeCombo();

    void DrawFilters();
    void DrawDiffResults();
    void DrawTypedResults();

    uintptr_t ResolveAddress(const ScanSession& session, size_t offset);
    void DrawAddressCell(uintptr_t address, int rowIndex);
    std::string FormatTypedValue(uint64_t raw, ScanDataType dt);

    // Region.
    char m_BaseBuf[32] = "0";
    mutable char m_SizeBuf[32] = "";
    int  m_DelayMs = 500;

    // Scan mode.
    int  m_ModeIndex = 0;
    int  m_DataTypeIndex = 0;

    char m_ValueABuf[32] = "0";
    char m_ValueBBuf[32] = "0";
    char m_BitMaskBuf[32] = "0";
    char m_BitPatternBuf[32] = "0";
    int  m_StabilizeRounds = 3;

    // Filters.
    char m_FilterFromBuf[32] = "0x0";
    char m_FilterToBuf[32] = "";
    bool m_FilterByBefore = false;
    int  m_FilterBeforeVal = 0x00;
    bool m_FilterByAfter = false;
    int  m_FilterAfterVal = 0x01;

    int m_SelectedRow = -1;

    // Static tables.
    struct ModeEntry 
    { 
        const char* Label; 
        ScanMode Mode; 
        bool NeedsBefore; 
        bool NeedsValueA; 
        bool NeedsValueB; 
        bool NeedsBitMask; 
        bool NeedsStabilize; 
    };

    struct TypeEntry 
    { 
        const char* Label; 
        ScanDataType Type; 
    };

    struct SizeEntry 
    { 
        const char* Label; 
        size_t Size; 
    };

    static const ModeEntry  s_Modes[10];
    static const TypeEntry  s_DataTypes[8];
    static const SizeEntry  s_KnownSizes[3];

    uint64_t ParseValueBuf(const char* buf, ScanDataType type) const;
    const ModeEntry& CurrentMode() const;
    ScanDataType CurrentDataType() const;
    bool IsTypedMode() const;
};