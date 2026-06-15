#include "pch.h"

#include "System_MapReader.h"

#include "Core/Types/Sources/MapReader/MapTypes.h"
#include "Core/Types/Sources/MapReader/MapMagics.h"
#include "Core/Types/Sources/MapReader/MapOffsets.h"
#include "Core/Types/Sources/MapReader/ResolvedTag.h"

#include "Core/States/Sources/MapReader/State_MapReader.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include <zlib.h>

// This '.map' reader was based on:
// Assembly: https://github.com/xboxchaos/assembly
// Reclaimer: https://github.com/Gravemind2401/Reclaimer

// Note: offset mask [3] is not yet used. 
// It possibly corresponds to locales, that requires 
// its own To...Offset() conversion function.

void System_MapReader::LoadMap(std::string& filePath)
{
	m_Deps.State_MapReader.SetMapFilePath(filePath);

	FILE* file = this->OpenMapFile(filePath);
	if (!file)
	{
		m_Deps.System_Logs.Log("[MapReader] ERROR:"
			" Could not open map file.");
		return;
	}

	bool isValid = this->ReadHeader(file) &&
		this->ReadIndexHeader(file) &&
		this->ReadFileNames(file);

	fclose(file);

	if (isValid)
	{
		m_Deps.System_Logs.Log("[MapReader] INFO: Map loaded.");
	}
	else
	{
		m_Deps.System_Logs.Log("[MapReader] ERROR:"
			" Failed to load map.");
	}

	m_Deps.State_MapReader.SetLoaded(isValid);
}

bool System_MapReader::ReadHeader(FILE* file)
{
	namespace MH = MapOffsets::MapHeader;

	fseek(file, MH::k_VirtualBaseAddress, SEEK_SET);
	if (fread(&m_VirtualBaseAddress, sizeof(m_VirtualBaseAddress), 1, file) != 1)
	{
		return false;
	}

	uint64_t indexHeaderAddress = 0;
	if (fread(&indexHeaderAddress, sizeof(indexHeaderAddress), 1, file) != 1)
	{
		return false;
	}

	fseek(file, MH::k_FileTableCount, SEEK_SET);
	if (fread(&m_FileTableCount, sizeof(m_FileTableCount), 1, file) != 1)
	{
		return false;
	}

	if (fread(&m_FileTableOffset, sizeof(m_FileTableOffset), 1, file) != 1)
	{
		return false;
	}

	if (fread(&m_FileTableSize, sizeof(m_FileTableSize), 1, file) != 1)
	{
		return false;
	}

	if (fread(&m_FileIndexTableOffset, sizeof(m_FileIndexTableOffset), 1, file) != 1)
	{
		return false;
	}

	uint32_t offsetMasks[MH::OffsetMasks::k_Count] = {};
	fseek(file, MH::OffsetMasks::k_Base, SEEK_SET);
	if (fread(offsetMasks, sizeof(uint32_t), MH::OffsetMasks::k_Count, file) != MH::OffsetMasks::k_Count)
	{
		return false;
	}

	m_DebugOffsetMask = offsetMasks[0];
	m_ResourceOffsetMask = offsetMasks[1];
	m_TagOffsetMask = offsetMasks[2];

	Map_Header_Sections sections[MH::Sections::k_Count] = {};
	fseek(file, MH::Sections::k_Base, SEEK_SET);
	if (fread(sections, sizeof(Map_Header_Sections), MH::Sections::k_Count, file) != MH::Sections::k_Count)
	{
		return false;
	}

	m_DebugVirtualAddress = sections[0].VirtualAddress;
	m_ResourceVirtualAddress = sections[1].VirtualAddress;
	m_TagVirtualAddress = sections[2].VirtualAddress;

	m_TagSectionFileOffset = (int64_t)m_TagVirtualAddress +
		(int32_t)m_TagOffsetMask;

	m_IndexHeaderFileOffset = this->ToFileOffset(
		indexHeaderAddress);

	return true;
}

bool System_MapReader::ReadIndexHeader(FILE* file)
{
	namespace ITH = MapOffsets::IndexTableHeader;

	auto seek = [&](size_t fieldOffset) {
		return fseek(file, (long)(m_IndexHeaderFileOffset +
			fieldOffset), SEEK_SET);
	};

	uint32_t magic = 0;
	seek(ITH::k_Magic);
	if (fread(&magic, sizeof(magic), 1, file) != 1)
	{
		return false;
	}

	if (magic != MapMagics::k_IndexHeaderMagic)
	{
		return false;
	}

	int32_t groupCount = 0;
	seek(ITH::k_NumberOfTagGroups);
	if (fread(&groupCount, sizeof(groupCount), 1, file) != 1)
	{
		return false;
	}

	uint64_t groupTableAddress = 0;
	seek(ITH::k_TagGroupTableAddress);
	if (fread(&groupTableAddress,
		sizeof(groupTableAddress), 1, file) != 1)
	{
		return false;
	}

	int32_t tagCount = 0;
	seek(ITH::k_NumberOfTags);
	if (fread(&tagCount, sizeof(tagCount), 1, file) != 1)
	{
		return false;
	}

	uint64_t tagTableAddress = 0;
	seek(ITH::k_TagTableAddress);
	if (fread(&tagTableAddress, sizeof(tagTableAddress), 1, file) != 1)
	{
		return false;
	}

	if (!this->ReadTagGroups(file, this->ToFileOffset(
		groupTableAddress), groupCount))
	{
		return false;
	}

	if (!this->ReadTags(file, this->ToFileOffset(
		tagTableAddress), tagCount))
	{
		return false;
	}

	return true;
}

bool System_MapReader::ReadTagGroups(
	FILE* file, int64_t fileOffset, int32_t count)
{
	if (count <= 0) return false;

	m_Deps.State_MapReader.ResizeGroups(count);
	fseek(file, (long)fileOffset, SEEK_SET);
	if (fread(m_Deps.State_MapReader.GetGroupsData(), sizeof(Map_TagTableGroupEntry), count, file) != (size_t)count)
	{
		return false;
	}

	return true;
}

bool System_MapReader::ReadTags(FILE* file, int64_t fileOffset, int32_t count)
{
	if (count <= 0) return false;

	m_Deps.State_MapReader.ResizeTags(count);
	fseek(file, (long)fileOffset, SEEK_SET);
	if (fread(m_Deps.State_MapReader.GetTagsData(), sizeof(Map_TagTableEntry), count, file) != (size_t)count)
	{
		return false;
	}

	return true;
}

bool System_MapReader::ReadFileNames(FILE* f) const
{
	if (m_FileTableCount <= 0) return true;

	uint32_t nameIndexOffset =
		(uint32_t)this->ToDebugOffset(m_FileIndexTableOffset);

	uint32_t nameDataOffset =
		(uint32_t)this->ToDebugOffset(m_FileTableOffset);

	m_Deps.State_MapReader.ResizeNameOffsets(m_FileTableCount);
	fseek(f, nameIndexOffset, SEEK_SET);
	if (fread(m_Deps.State_MapReader.GetNameOffsetsData(), sizeof(int32_t), m_FileTableCount, f) != (size_t)m_FileTableCount)
	{
		return false;
	}

	m_Deps.State_MapReader.ResizeNameData(m_FileTableSize);
	fseek(f, nameDataOffset, SEEK_SET); 
	if (fread(m_Deps.State_MapReader.GetNameData(), sizeof(char), m_FileTableSize, f) != (size_t)m_FileTableSize)
	{
		return false;
	}

	return true;
}

ResolvedTag System_MapReader::ResolveHandle(uint32_t handle) const
{
	if (!m_Deps.State_MapReader.IsLoaded())
	{
		m_Deps.System_Logs.Log("[MapReader] WARNING:"
			" No map loaded.");
		return {};
	}

	uint16_t salt = (handle >> 16) & 0xFFFF;
	uint16_t index = handle & 0xFFFF;

	if (salt == 0xFFFF || index == 0xFFFF)
	{
		m_Deps.System_Logs.Log("[MapReader] WARNING:"
			" Salt or Index are invalid.");
		return {};
	}

	if (index >= m_Deps.State_MapReader.GetTagsSize())
	{
		m_Deps.System_Logs.Log("[MapReader] WARNING:"
			" Index %d is out of range.", index);
		return {};
	}

	const Map_TagTableEntry& tagEntry = 
		m_Deps.State_MapReader.GetTag(index);
	if (tagEntry.DatumIndexSalt == 0xFFFF)
	{
		m_Deps.System_Logs.Log("[MapReader] WARNING:"
			" DatumSalt is invalid.");
		return {};
	}

	if (tagEntry.DatumIndexSalt != salt)
	{
		m_Deps.System_Logs.Log("[MapReader] WARNING:"
			" DatumSalt mismatch. Expected: %u, Got: %u",
			salt, tagEntry.DatumIndexSalt);
		return {};
	}

	ResolvedTag tag;
	tag.IsValid = true;

	if (tagEntry.TagGroupIndex >= 0 && tagEntry.TagGroupIndex < 
		(uint16_t)m_Deps.State_MapReader.GetGroupsSize())
	{
		uint32_t magic = m_Deps.State_MapReader.GetGroupMagic(
			tagEntry.TagGroupIndex);

		tag.FourCC = this->MagicToString(magic);
	}

	tag.TagName = m_Deps.State_MapReader.GetTagName(index);
	return tag;
}

std::string System_MapReader::MagicToString(int32_t magic) const
{
	char str[5]{};

	str[0] = (magic >> 24) & 0xFF;
	str[1] = (magic >> 16) & 0xFF;
	str[2] = (magic >> 8) & 0xFF;
	str[3] = (magic) & 0xFF;
	str[4] = 0;

	return std::string(str);
}

int64_t System_MapReader::GetTagOffset(
	int32_t tagIndex) const
{
	if (tagIndex < 0 || tagIndex >= 
		(int32_t)m_Deps.State_MapReader.GetTagsSize())
	{
		return -1;
	}

	const Map_TagTableEntry& entry = 
		m_Deps.State_MapReader.GetTag(tagIndex);
	if (entry.MemoryAddress == 0)
	{
		return -1;
	}

	return this->ToFileOffset(
		this->Expand(entry.MemoryAddress));
}

FILE* System_MapReader::OpenMapFile(
	const std::string& filePath) const
{
	if (filePath.empty()) return nullptr;
	FILE* file = nullptr;
	fopen_s(&file, filePath.c_str(), "rb");
	return file;
}

std::string System_MapReader::ResolveExternalCachePath(
	const std::string& mapPath) const
{
	std::string lower;
	lower.reserve(mapPath.size());
	for (char c : mapPath)
	{
		lower.push_back((char)std::tolower((unsigned char)c));
	}

	if (lower.find("shared") != std::string::npos)
	{
		return m_Deps.State_MapReader.GetSharedFilePath();
	}

	if (lower.find("campaign") != std::string::npos)
	{
		return m_Deps.State_MapReader.GetCampaignFilePath();
	}

	return {};
}

Map_TagTableEntry System_MapReader::ReadTagRef(
	FILE* file, int64_t tagRefOffset) const
{
	if (tagRefOffset < 0) return {};

	if (_fseeki64(file, tagRefOffset + offsetof(Map_TagRef, DatumIndex), SEEK_SET) != 0)
	{
		return {};
	}

	uint32_t datumIndex = 0;
	if (fread(&datumIndex, sizeof(datumIndex), 1, file) != 1)
	{
		return {};
	}

	if (datumIndex == 0xFFFFFFFF || datumIndex == 0)
	{
		return {};
	}

	int32_t tagIndex = datumIndex & 0xFFFF;

	if (tagIndex < 0 || tagIndex >=
		(int32_t)m_Deps.State_MapReader.GetTagsSize())
	{
		return {};
	}

	return m_Deps.State_MapReader.GetTag(tagIndex);
}

std::string System_MapReader::ResolveTagRefName(
	const Map_TagRef& ref) const
{
	if (ref.DatumIndex == 0xFFFFFFFF || ref.DatumIndex == 0)
	{
		return {};
	}

	int32_t tagIndex = ref.DatumIndex & 0xFFFF;
	if (tagIndex < 0 || tagIndex >=
		(int32_t)m_Deps.State_MapReader.GetTagsSize())
	{
		return {};
	}

	return m_Deps.State_MapReader.GetTagName(tagIndex);
}

int64_t System_MapReader::ResolveTagOffset(
	const Map_TagTableEntry& tag) const
{
	if (tag.MemoryAddress == 0) return -1;
	return this->ToFileOffset(this->Expand(tag.MemoryAddress));
}

Map_TagBlock System_MapReader::ReadTagBlock(
	FILE* file, int64_t blockHeaderOffset) const
{
	Map_TagBlock block{};
	if (blockHeaderOffset < 0) return block;

	if (_fseeki64(file, blockHeaderOffset, SEEK_SET) == 0)
	{
		fread(&block, sizeof(Map_TagBlock), 1, file);
	}

	return block;
}

int64_t System_MapReader::ResolveBlockOffset(
	const Map_TagBlock& block) const
{
	if (block.EntryCount <= 0 || block.Pointer == 0) return -1;
	return this->ToFileOffset(this->Expand(block.Pointer));
}

std::vector<uint8_t> System_MapReader::ReadData(
	int64_t fileOffset, int32_t size) const
{
	const std::string& filePath = 
		m_Deps.State_MapReader.GetMapFilePath();

	FILE* file = this->OpenMapFile(filePath);
	if (!file) return {};

	std::vector<uint8_t> buffer(size);
	_fseeki64(file, fileOffset, SEEK_SET);
	fread(buffer.data(), 1, size, file);
	fclose(file);
	return buffer;
}

std::vector<uint8_t> System_MapReader::ReadSegment(
	int64_t fileOffset, int32_t compressedSize, 
	int32_t decompressedSize, int32_t segmentOffset, 
	int32_t segmentLength) const
{
	if (compressedSize == decompressedSize)
	{
		return this->ReadData(
			segmentOffset, segmentLength);
	}

	auto compressed = this->ReadData(
		fileOffset, compressedSize);
	if (compressed.empty()) return {};

	std::vector<uint8_t> decompressed(decompressedSize);

	z_stream stream{};
	if (inflateInit2(&stream, -15) != Z_OK) return {};

	stream.next_in = compressed.data();
	stream.avail_in = compressedSize;
	stream.next_out = decompressed.data();
	stream.avail_out = decompressedSize;

	int ret = inflate(&stream, Z_FINISH);
	inflateEnd(&stream);

	if (ret != Z_STREAM_END && ret != Z_OK) return {};

	int32_t length = (std::min)(segmentLength, 
		decompressedSize - segmentOffset);

	return std::vector<uint8_t>(
		decompressed.begin() + segmentOffset,
		decompressed.begin() + segmentOffset + length);
}

std::vector<uint8_t> System_MapReader::ReadDataFrom(
	const std::string& filePath, int64_t fileOffset, 
	int32_t size) const
{
	if (filePath.empty() || size <= 0) return {};
	
	FILE* file = this->OpenMapFile(filePath);

	std::vector<uint8_t> buffer(size);
	_fseeki64(file, fileOffset, SEEK_SET);
	size_t read = fread(buffer.data(), 1, (size_t)size, file);
	fclose(file);

	if (read < (size_t)size) buffer.resize(read);
	return buffer;
}

std::vector<uint8_t> System_MapReader::ReadSegmentFrom(
	const std::string& filePath, int64_t fileOffset, 
	int32_t compressedSize, int32_t decompressedSize,
	int32_t segmentOffset, int32_t segmentLength) const
{
	if (decompressedSize <= 0 || segmentOffset < 0) return {};

	if (compressedSize == decompressedSize)
	{
		int32_t length = (std::min)(segmentLength, 
			decompressedSize - segmentOffset);

		if (length <= 0) return {};

		return this->ReadDataFrom(filePath, 
			fileOffset + segmentOffset, length);
	}

	auto compressed = this->ReadDataFrom(
		filePath, fileOffset, compressedSize);

	if ((int32_t)compressed.size() < compressedSize)
	{
		return {};
	}

	std::vector<uint8_t> decompressed(decompressedSize);

	z_stream stream{};
	if (inflateInit2(&stream, -15) != Z_OK) return {};

	stream.next_in = compressed.data();
	stream.avail_in = compressedSize;
	stream.next_out = decompressed.data();
	stream.avail_out = decompressedSize;

	int ret = inflate(&stream, Z_FINISH);
	inflateEnd(&stream);

	if (ret != Z_STREAM_END && ret != Z_OK) return {};

	int32_t length = (std::min)(segmentLength, 
		decompressedSize - segmentOffset);
	if (length <= 0) return {};

	return std::vector<uint8_t>(decompressed.begin() + 
		segmentOffset, decompressed.begin() + 
		segmentOffset + length);
}

int64_t System_MapReader::Expand(uint32_t address) const
{
	if (address == 0 || address == 0xFFFFFFFF) return 0;
	return ((int64_t)address << 2) + k_ExpandMagic;
}

int64_t System_MapReader::ToFileOffset(
	int64_t virtualAddress) const
{
	return virtualAddress - (int64_t)m_VirtualBaseAddress +
		m_TagSectionFileOffset;
}

int64_t System_MapReader::ToDebugOffset(int64_t pointer) const
{
	return pointer + m_DebugOffsetMask;
}

int64_t System_MapReader::ToResourceOffset(int64_t blockOffset) const
{
	return blockOffset + (int64_t)m_ResourceVirtualAddress + 
		(int32_t)m_ResourceOffsetMask;
}

void System_MapReader::Cleanup()
{
	m_VirtualBaseAddress = 0;
	m_IndexHeaderFileOffset = 0;
	m_FileTableCount = 0;
	m_FileTableOffset = 0;
	m_FileTableSize = 0;
	m_FileIndexTableOffset = 0;
	m_DebugOffsetMask = 0;
	m_ResourceOffsetMask = 0;
	m_TagOffsetMask = 0;
	m_DebugVirtualAddress = 0;
	m_ResourceVirtualAddress = 0;
	m_TagVirtualAddress = 0;
	m_TagSectionFileOffset = 0;

	m_Deps.State_MapReader.Cleanup();

	m_Deps.System_Logs.Log("[MapReader] INFO:"
		" Cleanup completed.");
}