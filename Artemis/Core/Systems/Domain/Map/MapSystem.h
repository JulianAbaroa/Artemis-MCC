#pragma once

#include "Core/Common/Types/Domain/Map/MapTypes.h"
#include <string>
#include <vector>

class MapSystem
{
public:
	bool LoadMap(const std::string& path);
	void Cleanup();

	// --- Tag Table Queries ---

	TagInfo ResolveHandle(uint32_t handle) const;
	int64_t GetTagMetaOffset(const TagInfo& info) const;
	int64_t GetTagMetaOffsetByIndex(int32_t tagIndex) const;

	// --- Shared Parsing Infrastructure ---

	FILE* OpenMapFile() const;

	Map_TagTableEntry ReadTagRef(FILE* file, int64_t tagRefOffset) const;
	int64_t ResolveTagOffset(const Map_TagTableEntry& tag) const;

	Map_TagBlock ReadTagBlock(FILE* file, int64_t blockHeaderOffset) const;
	int64_t ResolveBlockOffset(const Map_TagBlock& block) const;

	// --- Address Arithmetic ---

	int64_t Expand(uint32_t address) const;
	int64_t ToFileOffset(int64_t virtualAddress) const;
	int64_t ToDebugOffset(int64_t pointer) const;

private:
	// --- .MAP Parsing ---

	bool ReadHeader(FILE* file);
	bool ReadIndexHeader(FILE* file);
	bool ReadTagGroups(FILE* file, int64_t fileOffset, int32_t count);
	bool ReadTags(FILE* file, int64_t fileOffset, int32_t count);
	bool ReadFileNames(FILE* file) const;

	std::string MagicToString(int32_t magic) const;

	// --- Cached Header Values ---

	uint32_t m_DebugOffsetMask		= 0;
	uint32_t m_TagOffsetMask		= 0;
	uint32_t m_DebugVirtualAddress	= 0;
	uint32_t m_TagVirtualAddress	= 0;
	int32_t  m_FileTableCount		= 0;
	int32_t  m_FileTableOffset		= 0;
	int32_t  m_FileTableSize		= 0;
	int32_t  m_FileIndexTableOffset = 0;
	uint64_t m_VirtualBaseAddress	= 0;
	int64_t  m_MetaFileOffset		= 0;
	int64_t  m_IndexHeaderFileOffset= 0;
};