export module UI.MemoryScanner.State;

import UI.MemoryScanner.Type;
import std;

export namespace UI::MemoryScanner::State
{
    // Inputs and selected row of the memory scanner window.
    class MemoryScannerUIStore
    {
    private:
        using RegionForm = UI::MemoryScanner::Type::RegionForm;
        using ScanForm = UI::MemoryScanner::Type::ScanForm;
        using FilterForm = UI::MemoryScanner::Type::FilterForm;

    public:
        // Marks that no result row is selected.
        static constexpr int k_NoRow{ -1 };

        MemoryScannerUIStore() = default;
        ~MemoryScannerUIStore() = default;

        MemoryScannerUIStore(const MemoryScannerUIStore&) = delete;
        auto operator=(const MemoryScannerUIStore&) -> MemoryScannerUIStore& = delete;

        // Returns the inputs of the scanned region.
        auto GetRegion() -> RegionForm&;

        // Returns the inputs of the scan.
        auto GetScan() -> ScanForm&;
        auto GetScan() const -> const ScanForm&;

        // Returns the inputs of the result filter.
        auto GetFilters() -> FilterForm&;

        // Returns the selected result row, k_NoRow when there is none.
        auto GetSelectedRow() const -> int;

        // Selects a result row.
        auto SetSelectedRow(int row) -> void;

        // Clears the selected row.
        auto ClearSelectedRow() -> void;

    private:
        RegionForm m_Region{};
        ScanForm m_Scan{};
        FilterForm m_Filters{};
        int m_SelectedRow{ k_NoRow };
    };
}