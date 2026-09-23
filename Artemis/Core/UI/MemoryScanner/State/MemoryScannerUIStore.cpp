module UI.MemoryScanner.State;

namespace UI::MemoryScanner::State
{
	auto MemoryScannerUIStore::GetRegion() -> RegionForm&
	{
		return m_Region;
	}

	auto MemoryScannerUIStore::GetScan() -> ScanForm&
	{
		return m_Scan;
	}

	auto MemoryScannerUIStore::GetScan() const -> const ScanForm&
	{
		return m_Scan;
	}

	auto MemoryScannerUIStore::GetFilters() -> FilterForm&
	{
		return m_Filters;
	}

	auto MemoryScannerUIStore::GetSelectedRow() const -> int
	{
		return m_SelectedRow;
	}

	auto MemoryScannerUIStore::SetSelectedRow(int row) -> void
	{
		m_SelectedRow = row;
	}

	auto MemoryScannerUIStore::ClearSelectedRow() -> void
	{
		m_SelectedRow = k_NoRow;
	}
}