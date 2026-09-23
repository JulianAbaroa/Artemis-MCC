module;

#include "External/imgui/imgui.h"
#include "External/imgui/imgui_internal.h"

module UI.MemoryScanner.System;

import Tables.Object.Type;
import Tables.Player.Type;
import UI.MemoryScanner.Type;
import std;

namespace
{
	using Mode = Platform::Memory::Type::Mode;
	using DataType = Platform::Memory::Type::DataType;
	using ModeEntry = UI::MemoryScanner::Type::ModeEntry;
	using TypeEntry = UI::MemoryScanner::Type::TypeEntry;
	using SizeEntry = UI::MemoryScanner::Type::SizeEntry;

	namespace ObjectSize = Tables::Object::Type::Size;
	namespace PlayerSize = Tables::Player::Type::Size;

	constexpr std::array<ModeEntry, 10> k_Modes
	{ {
		{ "Changed",      Mode::Changed,     true,  false, false, false, false },
		{ "Unchanged",    Mode::Unchanged,   true,  false, false, false, false },
		{ "Increased",    Mode::Increased,   true,  false, false, false, false },
		{ "Decreased",    Mode::Decreased,   true,  false, false, false, false },
		{ "Increased By", Mode::IncreasedBy, true,  true,  false, false, false },
		{ "Decreased By", Mode::DecreasedBy, true,  true,  false, false, false },
		{ "Exact Value",  Mode::ExactValue,  false, true,  false, false, false },
		{ "In Range",     Mode::InRange,     false, true,  true,  false, false },
		{ "Bit Mask",     Mode::BitMask,     false, false, false, true,  false },
		{ "Stabilized",   Mode::Stabilized,  true,  false, false, false, true  },
	} };

	constexpr std::array<TypeEntry, 8> k_DataTypes
	{ {
		{ "Bytes (raw)", DataType::Bytes   },
		{ "Int8",        DataType::Int8    },
		{ "UInt8",       DataType::UInt8   },
		{ "Int16",       DataType::Int16   },
		{ "UInt16",      DataType::UInt16  },
		{ "Int32",       DataType::Int32   },
		{ "UInt32",      DataType::UInt32  },
		{ "Float32",     DataType::Float32 },
	} };

	constexpr std::array<SizeEntry, 3> k_KnownSizes
	{ {
		{ "Object::Base",  ObjectSize::Base   },
		{ "Object::Biped", ObjectSize::Biped  },
		{ "Player::Base",  PlayerSize::Base   },
	} };

	constexpr int k_DefaultTypedIndex = 5;

	const ImVec4 k_ValueColor{ 0.4f, 0.7f, 1.0f, 1.0f };
	constexpr ImU32 k_SelectedRowColor = IM_COL32(50, 80, 150, 120);

	constexpr ImGuiTableFlags k_ResultTableFlags =
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

	auto AllowsBytes(Mode mode) -> bool
	{
		return mode == Mode::Changed || mode == Mode::Unchanged;
	}

	auto ParseHex(const char* text) -> std::uint64_t
	{
		return std::strtoull(text, nullptr, 16);
	}

	auto ParseAuto(const char* text) -> std::uint64_t
	{
		return std::strtoull(text, nullptr, 0);
	}

	auto ParseFloat(const char* text) -> float
	{
		return std::strtof(text, nullptr);
	}

	auto DrawSnapshotLabel(bool isEmpty, const char* pending, const char* done) -> void
	{
		ImGui::PushStyleColor(ImGuiCol_Text, isEmpty ?
			ImVec4{ 1.0f, 0.5f, 0.0f, 1.0f } :
			ImVec4{ 0.0f, 1.0f, 0.0f, 1.0f });

		ImGui::Text("%s", isEmpty ? pending : done);
		ImGui::PopStyleColor();
	}

	auto DrawRowSelectable(int row, int selectedRow) -> bool
	{
		const ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns |
			ImGuiSelectableFlags_AllowOverlap;

		return ImGui::Selectable("##row", row == selectedRow, flags, ImVec2(0.0f, 0.0f));
	}
}

namespace UI::MemoryScanner::System
{
	auto MemoryScannerUIService::Draw() -> void
	{
		this->DrawTopBar();

		ImGui::Separator();

		this->DrawModeConfig();

		ImGui::Separator();

		if (!this->IsTypedMode()) this->DrawFilters();

		ImGui::Spacing();

		if (this->IsTypedMode()) this->DrawTypedResults();
		else                     this->DrawDiffResults();
	}

	auto MemoryScannerUIService::DrawTopBar() -> void
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

	auto MemoryScannerUIService::DrawRegionInputs() -> void
	{
		auto& region = m_MemoryScannerUIStore.GetRegion();

		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Address");
		ImGui::SameLine();

		ImGui::SetNextItemWidth(150.0f);
		ImGui::InputText("##base", region.Base, sizeof(region.Base));

		ImGui::SameLine();

		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Size");
		ImGui::SameLine();

		ImGui::SetNextItemWidth(90.0f);
		ImGui::InputText("##size", region.Size, sizeof(region.Size));

		ImGui::SameLine();

		this->DrawKnownSizesCombo();

		ImGui::SameLine();

		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Delay (ms)");
		ImGui::SameLine();

		ImGui::SetNextItemWidth(60.0f);
		ImGui::InputInt("##delay", &region.DelayMs, 0);

		if (region.DelayMs < 0) region.DelayMs = 0;
	}

	auto MemoryScannerUIService::DrawKnownSizesCombo() -> void
	{
		if (!ImGui::BeginCombo("##knownsize", "",
			ImGuiComboFlags_NoPreview | ImGuiComboFlags_WidthFitPreview))
		{
			return;
		}

		auto& region = m_MemoryScannerUIStore.GetRegion();

		for (int i = 0; i < static_cast<int>(k_KnownSizes.size()); ++i)
		{
			ImGui::PushID(i);

			if (ImGui::Selectable(k_KnownSizes[i].Label, false))
			{
				const std::string text = std::format("0x{:X}", k_KnownSizes[i].Size);
				std::snprintf(region.Size, sizeof(region.Size), "%s", text.c_str());
			}

			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	auto MemoryScannerUIService::DrawTriggerResetButtons() -> void
	{
		const bool isScanning = m_MemoryScannerStore.IsScanning();

		if (isScanning) ImGui::BeginDisabled();

		if (ImGui::Button("Trigger")) this->DispatchScan();

		if (isScanning) ImGui::EndDisabled();

		ImGui::SameLine();

		if (ImGui::Button("Reset"))
		{
			m_MemoryScannerUIStore.ClearSelectedRow();
			m_MemoryScannerStore.Reset();
		}
	}

	auto MemoryScannerUIService::DispatchScan() -> void
	{
		auto& region = m_MemoryScannerUIStore.GetRegion();
		auto& scan = m_MemoryScannerUIStore.GetScan();

		const auto base = static_cast<std::uintptr_t>(ParseHex(region.Base));
		const auto size = static_cast<std::size_t>(ParseAuto(region.Size));
		const int delayMs = region.DelayMs;

		m_MemoryScannerStore.SetRegion("scan", base, size);
		m_MemoryScannerStore.SetDelayMs(delayMs);

		const DataType type = this->CurrentDataType();
		auto& service = m_MemoryScannerService;

		switch (this->CurrentMode().Mode)
		{
		case Mode::Changed:
			service.TriggerScan(delayMs);
			break;

		case Mode::Unchanged:
			service.TriggerUnchangedScan(delayMs);
			break;

		case Mode::Increased:
			service.TriggerIncreasedScan(type, delayMs);
			break;

		case Mode::Decreased:
			service.TriggerDecreasedScan(type, delayMs);
			break;

		case Mode::IncreasedBy:
			service.TriggerIncreasedByScan(type, ParseValue(scan.ValueA, type), delayMs);
			break;

		case Mode::DecreasedBy:
			service.TriggerDecreasedByScan(type, ParseValue(scan.ValueA, type), delayMs);
			break;

		case Mode::ExactValue:
			if (type == DataType::Float32)
				service.TriggerExactScanFloat(ParseFloat(scan.ValueA), delayMs);
			else
				service.TriggerExactScan(type, ParseValue(scan.ValueA, type), delayMs);
			break;

		case Mode::InRange:
			if (type == DataType::Float32)
				service.TriggerInRangeScanFloat(ParseFloat(scan.ValueA),
					ParseFloat(scan.ValueB), delayMs);
			else
				service.TriggerInRangeScan(type, ParseValue(scan.ValueA, type),
					ParseValue(scan.ValueB, type), delayMs);
			break;

		case Mode::BitMask:
			service.TriggerBitMaskScan(
				static_cast<std::uint32_t>(ParseAuto(scan.BitMask)),
				static_cast<std::uint32_t>(ParseAuto(scan.BitPattern)), delayMs);
			break;

		case Mode::Stabilized:
			service.TriggerStabilizedScan(type, scan.StabilizeRounds, delayMs);
			break;
		}
	}

	auto MemoryScannerUIService::DrawScanStatus() -> void
	{
		const Session& session = m_MemoryScannerStore.GetSession();
		const bool isScanning = m_MemoryScannerStore.IsScanning();
		const bool isTyped = this->IsTypedMode();

		if (session.Rounds.empty())
		{
			ImGui::TextDisabled("No rounds yet.");
			return;
		}

		const std::size_t resultCount = isTyped ?
			session.FinalMatches.size() : session.FinalDiffs.size();

		const int roundCount = static_cast<int>(session.Rounds.size());

		if (isScanning)
		{
			ImGui::TextColored({ 1.0f, 0.8f, 0.0f, 1.0f }, "Round %d - act now...", roundCount);
		}
		else
		{
			ImGui::TextColored(k_ValueColor, "%d round(s) - %zu match(es)",
				roundCount, resultCount);
		}

		if (ImGui::IsItemHovered()) this->DrawRoundHistoryTooltip(session, isTyped);
	}

	auto MemoryScannerUIService::DrawRoundHistoryTooltip(const Session& session,
		bool isTyped) -> void
	{
		ImGui::BeginTooltip();
		ImGui::Text("Round History");
		ImGui::Separator();

		for (int r = 0; r < static_cast<int>(session.Rounds.size()); ++r)
		{
			const auto& round = session.Rounds[r];

			if (round.IsUnchangedRound)
				ImGui::TextColored({ 1.0f, 0.4f, 0.4f, 1.0f }, "Round %d [UNCHANGED]:", r + 1);
			else
				ImGui::Text("Round %d:", r + 1);

			ImGui::SameLine();
			DrawSnapshotLabel(round.Before.Data.empty(), "Before...", "Before OK");

			ImGui::SameLine();
			DrawSnapshotLabel(round.After.Data.empty(), "After...", "After OK");

			if (round.IsComplete)
			{
				const std::size_t count = isTyped ? round.TypedDiffs.size() : round.Diffs.size();

				ImGui::SameLine();
				ImGui::TextColored(k_ValueColor, "(%zu matches)", count);
			}
		}

		ImGui::EndTooltip();
	}

	auto MemoryScannerUIService::DrawModeConfig() -> void
	{
		this->DrawModeCombo();

		ImGui::SameLine();

		this->DrawDataTypeCombo();

		auto& scan = m_MemoryScannerUIStore.GetScan();
		const ModeEntry& mode = this->CurrentMode();
		const bool isFloat = this->CurrentDataType() == DataType::Float32;
		const char* format = isFloat ? "float (e.g. 1.0)" : "hex or dec";

		if (mode.NeedsValueA)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("Value");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(120.0f);
			ImGui::InputText(mode.NeedsValueB ? "##valueA" : "##valueAsingle",
				scan.ValueA, sizeof(scan.ValueA));
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", format);
		}

		if (mode.NeedsValueB)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("to");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(120.0f);
			ImGui::InputText("##valueB", scan.ValueB, sizeof(scan.ValueB));
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", format);
		}

		if (mode.NeedsBitMask)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("Mask");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputText("##mask", scan.BitMask, sizeof(scan.BitMask));

			ImGui::SameLine();
			ImGui::TextDisabled("Pattern");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputText("##pattern", scan.BitPattern, sizeof(scan.BitPattern));
		}

		if (mode.NeedsStabilize)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("Rounds");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			ImGui::InputInt("##stabilize", &scan.StabilizeRounds, 1);
			if (scan.StabilizeRounds < 2) scan.StabilizeRounds = 2;
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Minimum consecutive unchanged rounds required");
		}
	}

	auto MemoryScannerUIService::DrawModeCombo() -> void
	{
		auto& scan = m_MemoryScannerUIStore.GetScan();

		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Mode");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(130.0f);

		if (!ImGui::BeginCombo("##mode", k_Modes[scan.ModeIndex].Label)) return;

		for (int i = 0; i < static_cast<int>(k_Modes.size()); ++i)
		{
			ImGui::PushID(i);
			const bool isSelected = (i == scan.ModeIndex);

			if (ImGui::Selectable(k_Modes[i].Label, isSelected))
			{
				scan.ModeIndex = i;

				if (!AllowsBytes(k_Modes[i].Mode) &&
					this->CurrentDataType() == DataType::Bytes)
				{
					scan.DataTypeIndex = k_DefaultTypedIndex;
				}
			}

			if (isSelected) ImGui::SetItemDefaultFocus();

			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	auto MemoryScannerUIService::DrawDataTypeCombo() -> void
	{
		auto& scan = m_MemoryScannerUIStore.GetScan();
		const int typeStart = AllowsBytes(this->CurrentMode().Mode) ? 0 : 1;

		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Type");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(130.0f);

		if (!ImGui::BeginCombo("##type", k_DataTypes[scan.DataTypeIndex].Label)) return;

		for (int i = typeStart; i < static_cast<int>(k_DataTypes.size()); ++i)
		{
			ImGui::PushID(i);
			const bool isSelected = (i == scan.DataTypeIndex);

			if (ImGui::Selectable(k_DataTypes[i].Label, isSelected)) scan.DataTypeIndex = i;

			if (isSelected) ImGui::SetItemDefaultFocus();

			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	auto MemoryScannerUIService::DrawFilters() -> void
	{
		auto& filters = m_MemoryScannerUIStore.GetFilters();

		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Offset");
		ImGui::SameLine();

		ImGui::SetNextItemWidth(80.0f);
		ImGui::InputText("##from", filters.From, sizeof(filters.From));

		ImGui::SameLine();

		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("to");
		ImGui::SameLine();

		ImGui::SetNextItemWidth(80.0f);
		ImGui::InputText("##to", filters.To, sizeof(filters.To));

		ImGui::SameLine();

		ImGui::Checkbox("Before", &filters.ByBefore);
		if (filters.ByBefore)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(45.0f);
			ImGui::InputInt("##bval", &filters.BeforeValue, 0);
			filters.BeforeValue = std::clamp(filters.BeforeValue, 0, 255);
		}

		ImGui::SameLine();

		ImGui::Checkbox("After", &filters.ByAfter);
		if (filters.ByAfter)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(45.0f);
			ImGui::InputInt("##aval", &filters.AfterValue, 0);
			filters.AfterValue = std::clamp(filters.AfterValue, 0, 255);
		}
	}

	auto MemoryScannerUIService::DrawDiffResults() -> void
	{
		const Session& session = m_MemoryScannerStore.GetSession();
		const auto& diffs = session.FinalDiffs;

		if (diffs.empty())
		{
			ImGui::TextDisabled("No diffs yet - trigger a scan.");
			return;
		}

		auto& filters = m_MemoryScannerUIStore.GetFilters();

		const auto filterFrom = static_cast<std::size_t>(ParseHex(filters.From));
		const auto filterTo = static_cast<std::size_t>(ParseHex(filters.To));
		const bool usesRange = filterTo > filterFrom;

		if (!ImGui::BeginTable("##difftable", 6, k_ResultTableFlags,
			ImVec2(0.0f, ImGui::GetContentRegionAvail().y)))
		{
			return;
		}

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 220.0f);
		ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Before", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableSetupColumn("After", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableSetupColumn("Dec Before", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Dec After", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableHeadersRow();

		const int selectedRow = m_MemoryScannerUIStore.GetSelectedRow();

		for (int d = 0; d < static_cast<int>(diffs.size()); ++d)
		{
			const auto& diff = diffs[d];

			if (usesRange && (diff.Offset < filterFrom || diff.Offset >= filterTo)) continue;
			if (filters.ByBefore && diff.Before != static_cast<std::uint8_t>(filters.BeforeValue)) continue;
			if (filters.ByAfter && diff.After != static_cast<std::uint8_t>(filters.AfterValue)) continue;

			ImGui::TableNextRow();
			if (d == selectedRow)
			{
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, k_SelectedRowColor);
			}

			ImGui::TableSetColumnIndex(0);
			ImGui::PushID(d);

			if (DrawRowSelectable(d, selectedRow)) m_MemoryScannerUIStore.SetSelectedRow(d);
			ImGui::SameLine();

			this->DrawAddressCell(ResolveAddress(session, diff.Offset), d);
			ImGui::PopID();

			ImGui::TableSetColumnIndex(1); ImGui::Text("0x%04zX", diff.Offset);
			ImGui::TableSetColumnIndex(2); ImGui::Text("%02X", diff.Before);
			ImGui::TableSetColumnIndex(3); ImGui::TextColored(k_ValueColor, "%02X", diff.After);
			ImGui::TableSetColumnIndex(4); ImGui::Text("%u", diff.Before);
			ImGui::TableSetColumnIndex(5); ImGui::TextColored(k_ValueColor, "%u", diff.After);
		}

		ImGui::EndTable();
	}

	auto MemoryScannerUIService::DrawTypedResults() -> void
	{
		const Session& session = m_MemoryScannerStore.GetSession();
		const auto& matches = session.FinalMatches;
		const DataType type = this->CurrentDataType();

		if (matches.empty())
		{
			ImGui::TextDisabled("No matches yet - trigger a scan.");
			return;
		}

		if (!ImGui::BeginTable("##typedtable", 4, k_ResultTableFlags,
			ImVec2(0.0f, ImGui::GetContentRegionAvail().y)))
		{
			return;
		}

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 220.0f);
		ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Before", ImGuiTableColumnFlags_WidthFixed, 160.0f);
		ImGui::TableSetupColumn("After", ImGuiTableColumnFlags_WidthFixed, 160.0f);
		ImGui::TableHeadersRow();

		const int selectedRow = m_MemoryScannerUIStore.GetSelectedRow();

		for (int d = 0; d < static_cast<int>(matches.size()); ++d)
		{
			const auto& match = matches[d];

			ImGui::TableNextRow();
			if (d == selectedRow)
			{
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, k_SelectedRowColor);
			}

			ImGui::TableSetColumnIndex(0);
			ImGui::PushID(d);

			if (DrawRowSelectable(d, selectedRow)) m_MemoryScannerUIStore.SetSelectedRow(d);
			ImGui::SameLine();

			this->DrawAddressCell(ResolveAddress(session, match.Offset), d);
			ImGui::PopID();

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("0x%04zX", match.Offset);

			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("%s", FormatTypedValue(match.ValueBefore, type).c_str());

			ImGui::TableSetColumnIndex(3);
			ImGui::TextColored(k_ValueColor, "%s", FormatTypedValue(match.ValueAfter, type).c_str());
		}

		ImGui::EndTable();
	}

	auto MemoryScannerUIService::DrawAddressCell(std::uintptr_t address, int rowIndex) -> void
	{
		const std::string text = std::format("0x{:X}", address);

		const float buttonWidth = ImGui::CalcTextSize("Copy").x +
			ImGui::GetStyle().FramePadding.x * 2.0f;

		const float columnRight = ImGui::GetCursorPosX() +
			ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize;

		ImGui::TextUnformatted(text.c_str());
		ImGui::SameLine();
		ImGui::SetCursorPosX(columnRight - buttonWidth);

		if (ImGui::SmallButton("Copy##cp"))
		{
			ImGui::SetClipboardText(text.c_str());
			m_MemoryScannerUIStore.SetSelectedRow(rowIndex);
		}
	}

	auto MemoryScannerUIService::CurrentMode() const -> const ModeEntry&
	{
		return k_Modes[m_MemoryScannerUIStore.GetScan().ModeIndex];
	}

	auto MemoryScannerUIService::CurrentDataType() const -> DataType
	{
		return k_DataTypes[m_MemoryScannerUIStore.GetScan().DataTypeIndex].Type;
	}

	auto MemoryScannerUIService::IsTypedMode() const -> bool
	{
		return this->CurrentDataType() != DataType::Bytes;
	}

	auto MemoryScannerUIService::ResolveAddress(const Session& session,
		std::size_t offset) -> std::uintptr_t
	{
		return session.Rounds.empty() ?
			0 : session.Rounds[0].Before.BaseAddress + offset;
	}

	auto MemoryScannerUIService::FormatTypedValue(std::uint64_t raw, DataType type) -> std::string
	{
		switch (type)
		{
		case DataType::Int8:
			return std::format("{} (0x{:02X})", static_cast<std::int8_t>(raw), static_cast<std::uint8_t>(raw));

		case DataType::UInt8:
			return std::format("{} (0x{:02X})", static_cast<std::uint8_t>(raw), static_cast<std::uint8_t>(raw));

		case DataType::Int16:
			return std::format("{} (0x{:04X})", static_cast<std::int16_t>(raw), static_cast<std::uint16_t>(raw));

		case DataType::UInt16:
			return std::format("{} (0x{:04X})", static_cast<std::uint16_t>(raw), static_cast<std::uint16_t>(raw));

		case DataType::Int32:
			return std::format("{} (0x{:08X})", static_cast<std::int32_t>(raw), static_cast<std::uint32_t>(raw));

		case DataType::UInt32:
			return std::format("{} (0x{:08X})", static_cast<std::uint32_t>(raw), static_cast<std::uint32_t>(raw));

		case DataType::Float32:
		{
			const auto bits = static_cast<std::uint32_t>(raw);
			return std::format("{:.6f} (0x{:08X})", std::bit_cast<float>(bits), bits);
		}

		default:
			return std::format("0x{:02X}", static_cast<std::uint8_t>(raw));
		}
	}

	auto MemoryScannerUIService::ParseValue(const char* text, DataType type) -> std::uint64_t
	{
		if (type == DataType::Float32)
		{
			return std::bit_cast<std::uint32_t>(ParseFloat(text));
		}

		return ParseAuto(text);
	}
}