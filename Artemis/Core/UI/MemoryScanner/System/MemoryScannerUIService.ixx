export module UI.MemoryScanner.System;

import Platform.Memory.Type;
import Platform.Memory.State;
import Platform.Memory.System;
import UI.MemoryScanner.Type;
import UI.MemoryScanner.State;
import std;

export namespace UI::MemoryScanner::System
{
    // Draws the memory scanner window with the scan inputs and the results.
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
            m_MemoryScannerUIStore(memoryScannerUIStore) {}
        ~MemoryScannerUIService() = default;

        MemoryScannerUIService(const MemoryScannerUIService&) = delete;
        auto operator=(const MemoryScannerUIService&) -> MemoryScannerUIService& = delete;

        // Draws the memory scanner window content.
        auto Draw() -> void;

    private:
        MemoryScannerStore& m_MemoryScannerStore;
        MemoryScannerService& m_MemoryScannerService;
        MemoryScannerUIStore& m_MemoryScannerUIStore;

        // Draws the region inputs, the scan buttons and the scan status.
        auto DrawTopBar() -> void;

        // Draws the address, size and delay inputs.
        auto DrawRegionInputs() -> void;

        // Draws the combo that fills the size with a known structure size.
        auto DrawKnownSizesCombo() -> void;

        // Draws the buttons that trigger a scan or reset the session.
        auto DrawTriggerResetButtons() -> void;

        // Draws the round count and the match count of the session.
        auto DrawScanStatus() -> void;

        // Draws the tooltip with the state of each round.
        auto DrawRoundHistoryTooltip(const Session& session, bool isTyped) -> void;

        // Starts the scan of the selected mode on the configured region.
        auto DispatchScan() -> void;

        // Draws the mode, the type and the value inputs.
        auto DrawModeConfig() -> void;

        // Draws the combo of the scan modes.
        // note: Switches to a typed data type when the mode does not allow raw bytes.
        auto DrawModeCombo() -> void;

        // Draws the combo of the data types.
        auto DrawDataTypeCombo() -> void;

        // Draws the offset and byte filters of the byte results.
        auto DrawFilters() -> void;

        // Draws the table of the byte differences.
        auto DrawDiffResults() -> void;

        // Draws the table of the typed matches.
        auto DrawTypedResults() -> void;

        // Draws the address of a row with its copy button.
        auto DrawAddressCell(std::uintptr_t address, int rowIndex) -> void;

        // Returns the selected scan mode.
        auto CurrentMode() const -> const ModeEntry&;

        // Returns the selected data type.
        auto CurrentDataType() const -> DataType;

        // Checks whether the selected data type is not raw bytes.
        auto IsTypedMode() const -> bool;

        // Returns the address of an offset inside the scanned region.
        // return: 0 if the session has no rounds.
        static auto ResolveAddress(const Session& session, std::size_t offset) -> std::uintptr_t;

        // Formats a raw value as the data type with its hexadecimal form.
        static auto FormatTypedValue(std::uint64_t raw, DataType type) -> std::string;

        // Parses the text as a value of the data type.
        // note: Floats are returned as their bit pattern.
        static auto ParseValue(const char* text, DataType type) -> std::uint64_t;
    };
}