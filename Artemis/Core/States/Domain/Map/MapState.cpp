#include "pch.h"
#include "Core/States/Domain/Map/MapState.h"

bool MapState::IsLoaded() const { return m_IsLoaded.load(); }
void MapState::SetLoaded(bool value) { m_IsLoaded.store(value); }

std::string MapState::GetMapFilePath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_MapFilePath;
}

void MapState::SetMapFilePath(std::string path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_MapFilePath = path;
}

const Map_TagTableEntry& MapState::GetTag(int32_t index) const
{
	return m_Tags.at(index);
}

uint32_t MapState::GetGroupMagic(int16_t groupIndex) const
{
	if (groupIndex < 0 || groupIndex >= m_Groups.size()) return 0;
	return m_Groups[groupIndex].Magic;
}

std::string MapState::GetTagName(int32_t index) const
{
	if (index < 0 || index >= (int32_t)m_NameOffsets.size())
	{
		return "";
	}
	
	int32_t offset = m_NameOffsets[index];
	if (offset == -1 || offset >= (int32_t)m_NameData.size())
	{
		return "unknown";
	}

	return std::string(&m_NameData[offset]);
}

Map_TagTableGroupEntry* MapState::GetGroupsData() { return m_Groups.data(); }
Map_TagTableEntry* MapState::GetTagsData() { return m_Tags.data(); }
int32_t* MapState::GetNameOffsetsData() { return m_NameOffsets.data(); }
char* MapState::GetNameData() { return m_NameData.data(); }

size_t MapState::GetTagsSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Tags.size();
}

size_t MapState::GetGroupsSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Groups.size();
}

size_t MapState::GetNameOffsetsSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_NameOffsets.size();
}

void MapState::ResizeTags(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Tags.resize(count);
}

void MapState::ResizeGroups(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Groups.resize(count);
}

void MapState::ResizeNameOffsets(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_NameOffsets.resize(count);
}

void MapState::ResizeNameData(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_NameData.resize(count);
}

void MapState::Cleanup()
{
	m_IsLoaded.store(false);

	std::lock_guard<std::mutex> lock(m_Mutex);

	m_Tags.clear();
	m_Groups.clear();
	m_NameData.clear();
	m_NameOffsets.clear();
}