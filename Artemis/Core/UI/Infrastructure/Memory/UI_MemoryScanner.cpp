#include "pch.h"

// Header.
#include "UI_MemoryScanner.h"

// --- Types ---

#include "Core/Types/Domain/Object/ObjectSizes.h"
#include "Core/Types/Domain/Player/PlayerSizes.h"

#include "Core/Types/Infrastructure/MemoryScannerTypes.h"

// --- States ---

#include "Core/States/Infrastructure/Engine/Memory/State_MemoryScanner.h"

// --- Systems ---

#include "Core/Systems/Infrastructure/Engine/Memory/System_MemoryScanner.h"

// ImGui.
#include "External/imgui/imgui_internal.h"
#include "External/imgui/imgui.h"

#include <algorithm>
#include <cstring>
#include <array>

void UI_MemoryScanner::Draw()
{
    this->DrawTopBar();
    ImGui::Separator();
    this->DrawModeConfig();
    ImGui::Separator();

    if (!IsTypedMode()) this->DrawFilters();

    ImGui::Spacing();

    if (IsTypedMode()) this->DrawTypedResults();
    else this->DrawDiffResults();
}

// --- Top-bar ---

void UI_MemoryScanner::DrawTopBar()
{
    this->DrawRegionInputs();

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    this->DrawTriggerResetButtons();

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    this->DrawScanStatus();
}

void UI_MemoryScanner::DrawRegionInputs()
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Address");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::InputText("##base", m_BaseBuf, sizeof(m_BaseBuf));

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Size");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);
    ImGui::InputText("##size", m_SizeBuf, sizeof(m_SizeBuf));

    ImGui::SameLine();
    DrawKnownSizesCombo();

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Delay (ms)");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::InputInt("##delay", &m_DelayMs, 0);
    if (m_DelayMs < 0) m_DelayMs = 0;
}

void UI_MemoryScanner::DrawKnownSizesCombo()
{
    if (!ImGui::BeginCombo("##knownsize", "",
        ImGuiComboFlags_NoPreview | ImGuiComboFlags_WidthFitPreview))
        return;

    for (int i = 0; i < (int)(sizeof(s_KnownSizes) / sizeof(s_KnownSizes[0])); ++i)
    {
        ImGui::PushID(i);
        if (ImGui::Selectable(s_KnownSizes[i].Label, false))
            snprintf(m_SizeBuf, sizeof(m_SizeBuf), "0x%zX", s_KnownSizes[i].Size);
        ImGui::PopID();
    }

    ImGui::EndCombo();
}

void UI_MemoryScanner::DrawTriggerResetButtons()
{
    bool scanning = m_Deps.State_MemoryScanner.IsScanning();

    if (scanning) ImGui::BeginDisabled();

    if (ImGui::Button("Trigger"))
        DispatchScan();

    if (scanning) ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Reset"))
    {
        m_SelectedRow = -1;
        m_Deps.State_MemoryScanner.Reset();
    }
}

void UI_MemoryScanner::DispatchScan()
{
    uintptr_t base = (uintptr_t)strtoull(m_BaseBuf, nullptr, 16);
    size_t    size = (size_t)strtoull(m_SizeBuf, nullptr, 0);

    m_Deps.State_MemoryScanner.SetRegion("scan", base, size);
    m_Deps.State_MemoryScanner.SetDelayMs(m_DelayMs);

    const auto& mode = CurrentMode();
    ScanDataType dt = CurrentDataType();

    switch (mode.Mode)
    {
    case ScanMode::Changed:
        m_Deps.System_MemoryScanner.TriggerScan(m_DelayMs);
        break;
    case ScanMode::Unchanged:
        m_Deps.System_MemoryScanner.TriggerUnchangedScan(m_DelayMs);
        break;
    case ScanMode::Increased:
        m_Deps.System_MemoryScanner.TriggerIncreasedScan(dt, m_DelayMs);
        break;
    case ScanMode::Decreased:
        m_Deps.System_MemoryScanner.TriggerDecreasedScan(dt, m_DelayMs);
        break;
    case ScanMode::IncreasedBy:
        m_Deps.System_MemoryScanner.TriggerIncreasedByScan(
            dt, this->ParseValueBuf(m_ValueABuf, dt), m_DelayMs);
        break;
    case ScanMode::DecreasedBy:
        m_Deps.System_MemoryScanner.TriggerDecreasedByScan(
            dt, this->ParseValueBuf(m_ValueABuf, dt), m_DelayMs);
        break;
    case ScanMode::ExactValue:
        if (dt == ScanDataType::Float32)
            m_Deps.System_MemoryScanner.TriggerExactScanFloat(
                std::strtof(m_ValueABuf, nullptr), m_DelayMs);
        else
            m_Deps.System_MemoryScanner.TriggerExactScan(
                dt, this->ParseValueBuf(m_ValueABuf, dt), m_DelayMs);
        break;
    case ScanMode::InRange:
        if (dt == ScanDataType::Float32)
            m_Deps.System_MemoryScanner.TriggerInRangeScanFloat(
                std::strtof(m_ValueABuf, nullptr),
                std::strtof(m_ValueBBuf, nullptr), m_DelayMs);
        else
            m_Deps.System_MemoryScanner.TriggerInRangeScan(
                dt, this->ParseValueBuf(m_ValueABuf, dt),
                ParseValueBuf(m_ValueBBuf, dt), m_DelayMs);
        break;
    case ScanMode::BitMask:
        m_Deps.System_MemoryScanner.TriggerBitMaskScan(
            (uint32_t)strtoull(m_BitMaskBuf, nullptr, 0),
            (uint32_t)strtoull(m_BitPatternBuf, nullptr, 0),
            m_DelayMs);
        break;
    case ScanMode::Stabilized:
        m_Deps.System_MemoryScanner.TriggerStabilizedScan(
            dt, m_StabilizeRounds, m_DelayMs);
        break;
    }
}

void UI_MemoryScanner::DrawScanStatus()
{
    const auto& session = m_Deps.State_MemoryScanner.GetSession();
    bool scanning = m_Deps.State_MemoryScanner.IsScanning();
    const bool  isTyped = IsTypedMode();

    size_t resultCount;
    if (isTyped)
        resultCount = session.FinalMatches.size();
    else
        resultCount = session.FinalDiffs.size();

    if (session.Rounds.empty())
    {
        ImGui::TextDisabled("No rounds yet.");
        return;
    }

    if (scanning)
        ImGui::TextColored({ 1.0f, 0.8f, 0.0f, 1.0f },
            "Round %d — act now...", (int)session.Rounds.size());
    else
        ImGui::TextColored({ 0.4f, 0.7f, 1.0f, 1.0f },
            "%d round(s) — %zu match(es)", (int)session.Rounds.size(), resultCount);

    if (ImGui::IsItemHovered())
        DrawRoundHistoryTooltip(session, isTyped);
}

void UI_MemoryScanner::DrawRoundHistoryTooltip(const ScanSession& session, bool isTyped)
{
    ImGui::BeginTooltip();
    ImGui::Text("Round History");
    ImGui::Separator();

    for (int r = 0; r < (int)session.Rounds.size(); ++r)
    {
        const auto& round = session.Rounds[r];

        if (round.IsUnchangedRound)
            ImGui::TextColored({ 1.0f, 0.4f, 0.4f, 1.0f }, "Round %d [UNCHANGED]:", r + 1);
        else
            ImGui::Text("Round %d:", r + 1);

        ImGui::SameLine();

        auto snapshotLabel = [](bool empty, const char* pending, const char* ok) {
            ImGui::PushStyleColor(ImGuiCol_Text, empty
                ? ImVec4{ 1.0f, 0.5f, 0.0f, 1.0f }
            : ImVec4{ 0.0f, 1.0f, 0.0f, 1.0f });
            ImGui::Text("%s", empty ? pending : ok);
            ImGui::PopStyleColor();
            };

        snapshotLabel(round.Before.Data.empty(), "Before...", "Before OK");
        ImGui::SameLine();
        snapshotLabel(round.After.Data.empty(), "After...", "After OK");

        if (round.IsComplete)
        {
            ImGui::SameLine();
            const size_t count = isTyped ? round.TypedDiffs.size() : round.Diffs.size();
            ImGui::TextColored({ 0.4f, 0.7f, 1.0f, 1.0f }, "(%zu matches)", count);
        }
    }

    ImGui::EndTooltip();
}

// -- Mode Configuration ---

void UI_MemoryScanner::DrawModeConfig()
{
    DrawModeCombo();
    ImGui::SameLine();
    DrawDataTypeCombo();

    const auto& mode = CurrentMode();
    ScanDataType dt = CurrentDataType();
    const bool   isFloat = (dt == ScanDataType::Float32);
    const char* fmt = isFloat ? "float (e.g. 1.0)" : "hex or dec";

    if (mode.NeedsValueA)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("Value");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputText(mode.NeedsValueB ? "##valueA" : "##valueAsingle",
            m_ValueABuf, sizeof(m_ValueABuf));
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", fmt);
    }

    if (mode.NeedsValueB)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("to");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputText("##valueB", m_ValueBBuf, sizeof(m_ValueBBuf));
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", fmt);
    }

    if (mode.NeedsBitMask)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("Mask");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputText("##mask", m_BitMaskBuf, sizeof(m_BitMaskBuf));

        ImGui::SameLine();
        ImGui::TextDisabled("Pattern");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputText("##pattern", m_BitPatternBuf, sizeof(m_BitPatternBuf));
    }

    if (mode.NeedsStabilize)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("Rounds");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60.0f);
        ImGui::InputInt("##stabilize", &m_StabilizeRounds, 1);
        if (m_StabilizeRounds < 2) m_StabilizeRounds = 2;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Minimum consecutive unchanged rounds required");
    }
}

void UI_MemoryScanner::DrawModeCombo()
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Mode");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);

    if (!ImGui::BeginCombo("##mode", s_Modes[m_ModeIndex].Label))
        return;

    for (int i = 0; i < (int)(sizeof(s_Modes) / sizeof(s_Modes[0])); ++i)
    {
        ImGui::PushID(i);
        bool selected = (i == m_ModeIndex);
        if (ImGui::Selectable(s_Modes[i].Label, selected))
        {
            m_ModeIndex = i;
            bool needsType = s_Modes[i].Mode != ScanMode::Changed &&
                s_Modes[i].Mode != ScanMode::Unchanged;
            if (needsType && CurrentDataType() == ScanDataType::Bytes)
                m_DataTypeIndex = 5;
        }
        if (selected) ImGui::SetItemDefaultFocus();
        ImGui::PopID();
    }

    ImGui::EndCombo();
}

void UI_MemoryScanner::DrawDataTypeCombo()
{
    const bool bytesAllowed = s_Modes[m_ModeIndex].Mode == ScanMode::Changed ||
        s_Modes[m_ModeIndex].Mode == ScanMode::Unchanged;
    const int typeStart = bytesAllowed ? 0 : 1;

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Type");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);

    if (!ImGui::BeginCombo("##type", s_DataTypes[m_DataTypeIndex].Label))
        return;

    for (int i = typeStart; i < (int)(sizeof(s_DataTypes) / sizeof(s_DataTypes[0])); ++i)
    {
        ImGui::PushID(i);
        bool selected = (i == m_DataTypeIndex);
        if (ImGui::Selectable(s_DataTypes[i].Label, selected))
            m_DataTypeIndex = i;
        if (selected) ImGui::SetItemDefaultFocus();
        ImGui::PopID();
    }

    ImGui::EndCombo();
}

// --- Filters ---

void UI_MemoryScanner::DrawFilters()
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Offset");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::InputText("##from", m_FilterFromBuf, sizeof(m_FilterFromBuf));

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("to");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::InputText("##to", m_FilterToBuf, sizeof(m_FilterToBuf));

    ImGui::SameLine();
    ImGui::Checkbox("Before", &m_FilterByBefore);
    if (m_FilterByBefore)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(45.0f);
        ImGui::InputInt("##bval", &m_FilterBeforeVal, 0);
        m_FilterBeforeVal = std::clamp(m_FilterBeforeVal, 0, 255);
    }

    ImGui::SameLine();
    ImGui::Checkbox("After", &m_FilterByAfter);
    if (m_FilterByAfter)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(45.0f);
        ImGui::InputInt("##aval", &m_FilterAfterVal, 0);
        m_FilterAfterVal = std::clamp(m_FilterAfterVal, 0, 255);
    }
}

// --- Shared Table Helpers ---

uintptr_t UI_MemoryScanner::ResolveAddress(const ScanSession& session, size_t offset)
{
    return session.Rounds.empty()
        ? 0
        : session.Rounds[0].Before.BaseAddress + offset;
}

void UI_MemoryScanner::DrawAddressCell(uintptr_t address, int rowIndex)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)address);

    float buttonWidth = ImGui::CalcTextSize("Copy").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float columnRight = ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize;

    ImGui::Text("%s", buf);
    ImGui::SameLine();
    ImGui::SetCursorPosX(columnRight - buttonWidth);

    if (ImGui::SmallButton("Copy##cp"))
    {
        ImGui::SetClipboardText(buf);
        m_SelectedRow = rowIndex;
    }
}

// --- Diff Results ---

void UI_MemoryScanner::DrawDiffResults()
{
    const auto& session = m_Deps.State_MemoryScanner.GetSession();
    const auto& diffs = session.FinalDiffs;

    if (diffs.empty())
    {
        ImGui::TextDisabled("No diffs yet — trigger a scan.");
        return;
    }

    size_t filterFrom = (size_t)strtoull(m_FilterFromBuf, nullptr, 16);
    size_t filterTo = (size_t)strtoull(m_FilterToBuf, nullptr, 16);
    bool   useFilter = filterTo > filterFrom;

    constexpr ImGuiTableFlags kFlags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    if (!ImGui::BeginTable("##difftable", 6, kFlags,
        { 0.0f, ImGui::GetContentRegionAvail().y }))
        return;

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 220.0f);
    ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn("Before", ImGuiTableColumnFlags_WidthFixed, 120.0f);
    ImGui::TableSetupColumn("After", ImGuiTableColumnFlags_WidthFixed, 120.0f);
    ImGui::TableSetupColumn("Dec Before", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn("Dec After", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableHeadersRow();

    for (int d = 0; d < (int)diffs.size(); ++d)
    {
        if (useFilter && (diffs[d].Offset < filterFrom || diffs[d].Offset >= filterTo)) continue;
        if (m_FilterByBefore && diffs[d].Before != (uint8_t)m_FilterBeforeVal)          continue;
        if (m_FilterByAfter && diffs[d].After != (uint8_t)m_FilterAfterVal)           continue;

        const auto& diff = diffs[d];

        ImGui::TableNextRow();
        if (d == m_SelectedRow)
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(50, 80, 150, 120));

        ImGui::TableSetColumnIndex(0);
        ImGui::PushID(d);

        bool clicked = ImGui::Selectable("##row", d == m_SelectedRow,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
            { 0.0f, 0.0f });
        if (clicked) m_SelectedRow = d;
        ImGui::SameLine();

        DrawAddressCell(ResolveAddress(session, diff.Offset), d);
        ImGui::PopID();

        ImGui::TableSetColumnIndex(1); ImGui::Text("0x%04zX", diff.Offset);
        ImGui::TableSetColumnIndex(2); ImGui::Text("%02X", diff.Before);
        ImGui::TableSetColumnIndex(3); ImGui::TextColored({ 0.4f, 0.7f, 1.0f, 1.0f }, "%02X", diff.After);
        ImGui::TableSetColumnIndex(4); ImGui::Text("%u", diff.Before);
        ImGui::TableSetColumnIndex(5); ImGui::TextColored({ 0.4f, 0.7f, 1.0f, 1.0f }, "%u", diff.After);
    }

    ImGui::EndTable();
}

// --- Typed Results ---

std::string UI_MemoryScanner::FormatTypedValue(uint64_t raw, ScanDataType dt)
{
    char buf[64];
    switch (dt)
    {
    case ScanDataType::Int8:    snprintf(buf, sizeof(buf), "%d  (0x%02X)", (int8_t)raw, (uint8_t)raw);   break;
    case ScanDataType::UInt8:   snprintf(buf, sizeof(buf), "%u  (0x%02X)", (uint8_t)raw, (uint8_t)raw);   break;
    case ScanDataType::Int16:   snprintf(buf, sizeof(buf), "%d  (0x%04X)", (int16_t)raw, (uint16_t)raw);  break;
    case ScanDataType::UInt16:  snprintf(buf, sizeof(buf), "%u  (0x%04X)", (uint16_t)raw, (uint16_t)raw);  break;
    case ScanDataType::Int32:   snprintf(buf, sizeof(buf), "%d  (0x%08X)", (int32_t)raw, (uint32_t)raw);  break;
    case ScanDataType::UInt32:  snprintf(buf, sizeof(buf), "%u  (0x%08X)", (uint32_t)raw, (uint32_t)raw);  break;
    case ScanDataType::Float32: {
        float f; std::memcpy(&f, &raw, 4);
        snprintf(buf, sizeof(buf), "%.6f  (0x%08X)", f, (uint32_t)raw); break;
    }
    default:                    snprintf(buf, sizeof(buf), "0x%02X", (uint8_t)raw); break;
    }
    return buf;
}

void UI_MemoryScanner::DrawTypedResults()
{
    const auto& session = m_Deps.State_MemoryScanner.GetSession();
    const auto& matches = session.FinalMatches;
    ScanDataType dt = CurrentDataType();

    if (matches.empty())
    {
        ImGui::TextDisabled("No matches yet — trigger a scan.");
        return;
    }

    constexpr ImGuiTableFlags kFlags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    if (!ImGui::BeginTable("##typedtable", 4, kFlags,
        { 0.0f, ImGui::GetContentRegionAvail().y }))
        return;

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 220.0f);
    ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn("Before", ImGuiTableColumnFlags_WidthFixed, 160.0f);
    ImGui::TableSetupColumn("After", ImGuiTableColumnFlags_WidthFixed, 160.0f);
    ImGui::TableHeadersRow();

    for (int d = 0; d < (int)matches.size(); ++d)
    {
        const auto& match = matches[d];

        ImGui::TableNextRow();
        if (d == m_SelectedRow)
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(50, 80, 150, 120));

        ImGui::TableSetColumnIndex(0);
        ImGui::PushID(d);

        bool clicked = ImGui::Selectable("##row", d == m_SelectedRow,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
            { 0.0f, 0.0f });
        if (clicked) m_SelectedRow = d;
        ImGui::SameLine();

        DrawAddressCell(ResolveAddress(session, match.Offset), d);
        ImGui::PopID();

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("0x%04zX", match.Offset);

        ImGui::TableSetColumnIndex(2);
        ImGui::TextDisabled("%s", FormatTypedValue(match.ValueBefore, dt).c_str());

        ImGui::TableSetColumnIndex(3);
        ImGui::TextColored({ 0.4f, 0.7f, 1.0f, 1.0f },
            "%s", FormatTypedValue(match.ValueAfter, dt).c_str());
    }

    ImGui::EndTable();
}

uint64_t UI_MemoryScanner::ParseValueBuf(const char* buf, ScanDataType type) const
{
    if (type == ScanDataType::Float32)
    {
        float f = std::strtof(buf, nullptr);
        uint32_t raw;
        std::memcpy(&raw, &f, 4);
        return raw;
    }
    return (uint64_t)strtoull(buf, nullptr, 0);
}

const UI_MemoryScanner::ModeEntry& UI_MemoryScanner::CurrentMode() const { return s_Modes[m_ModeIndex]; }
ScanDataType UI_MemoryScanner::CurrentDataType() const { return s_DataTypes[m_DataTypeIndex].Type; }
bool UI_MemoryScanner::IsTypedMode() const { return CurrentDataType() != ScanDataType::Bytes; }

const UI_MemoryScanner::ModeEntry UI_MemoryScanner::s_Modes[10] =
{
    { "Changed",      ScanMode::Changed,      true,  false, false, false, false },
    { "Unchanged",    ScanMode::Unchanged,     true,  false, false, false, false },
    { "Increased",    ScanMode::Increased,     true,  false, false, false, false },
    { "Decreased",    ScanMode::Decreased,     true,  false, false, false, false },
    { "Increased By", ScanMode::IncreasedBy,   true,  true,  false, false, false },
    { "Decreased By", ScanMode::DecreasedBy,   true,  true,  false, false, false },
    { "Exact Value",  ScanMode::ExactValue,    false, true,  false, false, false },
    { "In Range",     ScanMode::InRange,       false, true,  true,  false, false },
    { "Bit Mask",     ScanMode::BitMask,       false, false, false, true,  false },
    { "Stabilized",   ScanMode::Stabilized,    true,  false, false, false, true  },
};

const UI_MemoryScanner::TypeEntry UI_MemoryScanner::s_DataTypes[8] =
{
    { "Bytes (raw)", ScanDataType::Bytes   },
    { "Int8",        ScanDataType::Int8    },
    { "UInt8",       ScanDataType::UInt8   },
    { "Int16",       ScanDataType::Int16   },
    { "UInt16",      ScanDataType::UInt16  },
    { "Int32",       ScanDataType::Int32   },
    { "UInt32",      ScanDataType::UInt32  },
    { "Float32",     ScanDataType::Float32 },
};

const UI_MemoryScanner::SizeEntry UI_MemoryScanner::s_KnownSizes[3] =
{
    { "Object::Base",  ObjectSizes::Base  },
    { "Object::Biped", ObjectSizes::Biped },
    { "Player::Base",  PlayerSizes::Base  },
};