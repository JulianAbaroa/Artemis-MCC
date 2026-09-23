module UI.Logs.State;

namespace UI::Logs::State
{
	auto LogsUIStore::GetSearchBuffer() -> char*
	{
		return m_SearchBuffer;
	}

	auto LogsUIStore::GetSearchBufferSize() const -> std::size_t
	{
		return sizeof(m_SearchBuffer);
	}

	auto LogsUIStore::GetSearchText() const -> std::string_view
	{
		return std::string_view(m_SearchBuffer);
	}

	auto LogsUIStore::ClearSearch() -> void
	{
		m_SearchBuffer[0] = '\0';
	}

	auto LogsUIStore::HasSelection() const -> bool
	{
		return m_SelectionStart != k_NoIndex && m_SelectionEnd != k_NoIndex;
	}

	auto LogsUIStore::IsIndexSelected(int index) const -> bool
	{
		if (!this->HasSelection()) return false;

		return index >= this->GetSelectionMin() && index <= this->GetSelectionMax();
	}

	auto LogsUIStore::SelectSingle(int index) -> void
	{
		m_SelectionStart = index;
		m_SelectionEnd = index;
	}

	auto LogsUIStore::ExtendSelection(int index) -> void
	{
		if (m_SelectionStart == k_NoIndex)
		{
			this->SelectSingle(index);
			return;
		}

		m_SelectionEnd = index;
	}

	auto LogsUIStore::ClearSelection() -> void
	{
		m_SelectionStart = k_NoIndex;
		m_SelectionEnd = k_NoIndex;
	}

	auto LogsUIStore::GetSelectionMin() const -> int
	{
		return (std::min)(m_SelectionStart, m_SelectionEnd);
	}

	auto LogsUIStore::GetSelectionMax() const -> int
	{
		return (std::max)(m_SelectionStart, m_SelectionEnd);
	}

	auto LogsUIStore::StartCopyAnimation(int index, float startTime) -> void
	{
		m_AnimatedIndex = index;
		m_AnimationStartTime = startTime;
	}

	auto LogsUIStore::StopCopyAnimation() -> void
	{
		m_AnimatedIndex = k_NoIndex;
	}

	auto LogsUIStore::GetAnimatedIndex() const -> int
	{
		return m_AnimatedIndex;
	}

	auto LogsUIStore::GetAnimationStartTime() const -> float
	{
		return m_AnimationStartTime;
	}
}