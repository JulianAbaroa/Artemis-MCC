export module UI.MemoryScanner.System;

import Platform.Memory.Type;
import Platform.Memory.State;
import Platform.Memory.System;
import UI.MemoryScanner.Type;
import UI.MemoryScanner.State;
import std;

export namespace UI::MemoryScanner::System
{
	class MemoryScannerUIService
	{
	private:
		using DataType = Platform::Memory::Type::DataType;
		using Session = Platform::Memory::Type::Session;
		using ModeEntry = UI::MemoryScanner::Type::ModeEntry;

		using MemoryScannerStore = Platform::Memory::State::MemoryScannerStore;
		using MemoryScannerService = Platform::Memory::System::MemoryScannerService;
		using MemoryScannerUIStore = UI::MemoryScanner::State::MemoryScannerUIStore;

	public:
		MemoryScannerUIService(MemoryScannerStore& memoryScannerStore,
			MemoryScannerService& memoryScannerService,
			MemoryScannerUIStore& memoryScannerUIStore) :
			m_MemoryScannerStore(memoryScannerStore),
			m_MemoryScannerService(memoryScannerService),
			m_MemoryScannerUIStore(memoryScannerUIStore) {
		}
		~MemoryScannerUIService() = default;

		MemoryScannerUIService(const MemoryScannerUIService&) = delete;
		MemoryScannerUIService& operator=(const MemoryScannerUIService&) = delete;

		auto Draw() -> void;

	private:
		MemoryScannerStore& m_MemoryScannerStore;
		MemoryScannerService& m_MemoryScannerService;
		MemoryScannerUIStore& m_MemoryScannerUIStore;

		auto DrawTopBar() -> void;
		auto DrawRegionInputs() -> void;
		auto DrawKnownSizesCombo() -> void;
		auto DrawTriggerResetButtons() -> void;
		auto DrawScanStatus() -> void;
		auto DrawRoundHistoryTooltip(const Session& session, bool isTyped) -> void;
		auto DispatchScan() -> void;

		auto DrawModeConfig() -> void;
		auto DrawModeCombo() -> void;
		auto DrawDataTypeCombo() -> void;

		auto DrawFilters() -> void;
		auto DrawDiffResults() -> void;
		auto DrawTypedResults() -> void;

		auto DrawAddressCell(std::uintptr_t address, int rowIndex) -> void;

		auto CurrentMode() const -> const ModeEntry&;
		auto CurrentDataType() const -> DataType;
		auto IsTypedMode() const -> bool;

		static auto ResolveAddress(const Session& session, std::size_t offset) -> std::uintptr_t;
		static auto FormatTypedValue(std::uint64_t raw, DataType type) -> std::string;
		static auto ParseValue(const char* text, DataType type) -> std::uint64_t;
	};
}