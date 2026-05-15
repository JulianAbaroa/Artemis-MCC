#pragma once

// Types.
#include "Core/Types/Domain/Map/MapTypes.h"

#include <cstdint>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>

class State_Map
{
public:
	bool IsLoaded() const;
	void SetLoaded(bool value);

	std::string GetMapFilePath() const;
	void SetMapFilePath(std::string path);

	const Map_TagTableEntry& GetTag(int32_t index) const;
	uint32_t GetGroupMagic(int16_t groupIndex) const;
	std::string GetTagName(int32_t index) const;

	Map_TagTableGroupEntry* GetGroupsData();
	Map_TagTableEntry* GetTagsData();
	int32_t* GetNameOffsetsData();
	char* GetNameData();

	size_t GetTagsSize() const;
	size_t GetGroupsSize() const;
	size_t GetNameOffsetsSize() const;

	void ResizeTags(int32_t count);
	void ResizeGroups(int32_t count);
	void ResizeNameOffsets(int32_t count);
	void ResizeNameData(int32_t count);

	void Cleanup();

private:
	std::atomic<bool> m_IsLoaded{ false };
	std::vector<Map_TagTableGroupEntry> m_Groups;
	std::vector<Map_TagTableEntry> m_Tags;
	std::vector<int32_t> m_NameOffsets;
	std::vector<char> m_NameData;
	std::string m_MapFilePath;

	mutable std::mutex m_Mutex;
};