export module UI.MemoryScanner.State;

import UI.MemoryScanner.Type;
import std;

export namespace UI::MemoryScanner::State
{
	class MemoryScannerUIStore
	{
	private:
		using RegionForm = UI::MemoryScanner::Type::RegionForm;
		using ScanForm = UI::MemoryScanner::Type::ScanForm;
		using FilterForm = UI::MemoryScanner::Type::FilterForm;

	public:
		static constexpr int k_NoRow = -1;

		MemoryScannerUIStore() = default;
		~MemoryScannerUIStore() = default;

		MemoryScannerUIStore(const MemoryScannerUIStore&) = delete;
		MemoryScannerUIStore& operator=(const MemoryScannerUIStore&) = delete;

		auto GetRegion() -> RegionForm&;
		auto GetScan() -> ScanForm&;
		auto GetScan() const -> const ScanForm&;
		auto GetFilters() -> FilterForm&;

		auto GetSelectedRow() const -> int;
		auto SetSelectedRow(int row) -> void;
		auto ClearSelectedRow() -> void;

	private:
		RegionForm m_Region{};
		ScanForm m_Scan{};
		FilterForm m_Filters{};
		int m_SelectedRow{ k_NoRow };
	};
}