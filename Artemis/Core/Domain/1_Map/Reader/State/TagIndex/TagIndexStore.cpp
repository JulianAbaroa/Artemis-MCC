module Map.Reader.State:TagIndex;

namespace Map::Reader::State
{
	auto TagIndexStore::GetTag(std::int32_t index) const -> const TagTableEntry&
	{
		return m_TagStore.at(index);
	}

	auto TagIndexStore::GetGroupMagic(std::int16_t groupIndex) const -> std::uint32_t
	{
		if (groupIndex < 0 || groupIndex >= m_Groups.size()) return 0;
		return m_Groups[groupIndex].Magic;
	}

	auto TagIndexStore::GetTagName(std::int32_t index) const -> std::string
	{
		if (index < 0 || index >= (std::int32_t)m_NameOffsets.size())
		{
			return "";
		}

		std::int32_t offset = m_NameOffsets[index];
		if (offset == -1 || offset >= (std::int32_t)m_NameData.size())
		{
			return "unknown";
		}

		return std::string(&m_NameData[offset]);
	}

	auto TagIndexStore::GetTagsData() const -> TagTableEntry*
	{
		return m_TagStore.data();
	}

	auto TagIndexStore::GetGroupsData() const -> TagTableGroupEntry*
	{
		return m_Groups.data();
	}

	auto TagIndexStore::GetNameOffsetsData() const -> std::int32_t*
	{
		return m_NameOffsets.data();
	}

	auto TagIndexStore::GetNameData() const -> char*
	{
		return m_NameData.data();
	}

	auto TagIndexStore::GetTagsSize() const -> std::size_t
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_TagStore.size();
	}

	auto TagIndexStore::GetGroupsSize() const -> std::size_t
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Groups.size();
	}

	auto TagIndexStore::GetNameOffsetsSize() const -> std::size_t
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_NameOffsets.size();
	}

	auto TagIndexStore::ResizeTags(std::int32_t count) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_TagStore.resize(count);
	}

	auto TagIndexStore::ResizeGroups(std::int32_t count) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Groups.resize(count);
	}

	auto TagIndexStore::ResizeNameData(std::int32_t count) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_NameData.resize(count);
	}

	auto TagIndexStore::ResizeNameOffsets(std::int32_t count) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_NameOffsets.resize(count);
	}

	auto TagIndexStore::Cleanup() -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		m_TagStore.clear();
		m_Groups.clear();
		m_NameOffsets.clear();
		m_NameData.clear();
	}
}