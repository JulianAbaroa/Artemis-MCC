#include "pch.h"

#include "System_MapReader.h"

#include "Core/Types/Map/MapTypes.h"
#include "Core/Types/Map/MapMagics.h"
#include "Core/Types/Map/MapOffsets.h"
#include "Core/Types/Map/ResolvedTag.h"

#include "Core/States/MapReader/State_MapReader.h"

#include "Core/Systems/Logs/System_Logs.h"

#include <zlib.h>

// This '.map' reader was based on:
// Assembly: https://github.com/xboxchaos/assembly
// Reclaimer: https://github.com/Gravemind2401/Reclaimer

// Entry point for loading a '.map' file into the system.
//
// Orchestrates the three-phase load sequence that must happen in order,
// as each phase depends on data resolved by the previous one:
//
// 1. ReadHeader(): reads the map header and resolves the address arithmetic
//    needed to navigate the rest of the file (virtual base, meta file offset,
//    index header file offset).
// 
// 2. ReadIndexHeader(): Uses the index header file offset to locate and load
//    the tag group table and tag table into memory.
//
// 3. ReadFileNames(): Uses the file table offsets to load the debug segment's
//    filename index and string data into memory.
//
// After these three phases, the tag table and filename data are fully resident
// in memory and the system is ready to resolve any tag by name or index via
// GetTagMetaOffset() / GetTagMetaOffsetByIndex().
void System_MapReader::LoadMap(std::string& path)
{
	m_Deps.State_Map.SetMapFilePath(path);

	FILE* file = this->OpenMapFile();
	if (!file)
	{
		m_Deps.System_Logs.Log("[MapSystem] ERROR: Could not open map file.");
		return;
	}

	bool isValid = this->ReadHeader(file) &&
		this->ReadIndexHeader(file) &&
		this->ReadFileNames(file);

	fclose(file);

	if (isValid)
	{
		m_Deps.System_Logs.Log("[MapSystem] INFO: Map loaded.");
	}
	else
	{
		m_Deps.System_Logs.Log("[MapSystem] ERROR: Failed to load map.");
	}

	m_Deps.State_Map.SetLoaded(isValid);
}

void System_MapReader::Cleanup()
{
	m_DebugOffsetMask = 0;
	m_TagOffsetMask = 0;
	m_DebugVirtualAddress = 0;
	m_TagVirtualAddress = 0;
	m_FileTableCount = 0;
	m_FileTableOffset = 0;
	m_FileTableSize = 0;
	m_FileIndexTableOffset = 0;
	m_VirtualBaseAddress = 0;
	m_MetaFileOffset = 0;
	m_IndexHeaderFileOffset = 0;

	m_Deps.State_Map.Cleanup();
	m_Deps.System_Logs.Log("[MapSystem] INFO: Cleanup completed.");
}

// --- Tag Table Queries ---

ResolvedTag System_MapReader::ResolveHandle(uint32_t handle) const
{
	if (!m_Deps.State_Map.IsLoaded())
	{
		m_Deps.System_Logs.Log("[MapSystem] WARNING: No map loaded.");
		return {};
	}

	// Isolate the Salt:
	// 0x1A2B3C4D >> 16 = 0x00001A2B
	// Clear upper bits and ensure 16-bit alignment.
	// 0x00001A2B & 0xFFFF = 0x00001A2B -> 0x1A2B.
	uint16_t salt = (handle >> 16) & 0xFFFF;

	// Isolate the Index:
	// 0x1A2B3C4D & 0xFFFF = 0x00003C4D -> 0x3C4D.
	uint16_t index = handle & 0xFFFF;

	// Validate handle integrity and bounds.
	if (salt == 0xFFFF && index == 0xFFFF)
	{
		m_Deps.System_Logs.Log("[MapSystem] WARNING: Salt or Index are invalid.");
		return {};
	}

	if (index >= m_Deps.State_Map.GetTagsSize())
	{
		m_Deps.System_Logs.Log("[MapSystem] WARNING: Index %d is out of range.", index);
		return {};
	}

	const Map_TagTableEntry& tagEntry = m_Deps.State_Map.GetTag(index);
	if (tagEntry.DatumIndexSalt == 0xFFFF)
	{
		m_Deps.System_Logs.Log("[MapSystem] WARNING: DatumSalt is invalid.");
		return {};
	}

	if (tagEntry.DatumIndexSalt != salt)
	{
		m_Deps.System_Logs.Log("[MapSystem] WARNING: DatumSalt mismatch."
			" Expected: %u, Got: %u", salt, tagEntry.DatumIndexSalt);
		return {};
	}

	ResolvedTag tag;
	tag.IsValid = true;

	if (tagEntry.TagGroupIndex >= 0 &&
		tagEntry.TagGroupIndex < (uint16_t)m_Deps.State_Map.GetGroupsSize())
	{
		// We get the class of this tag (e.g. scen, proj, vehi, ...)
		uint32_t magic = m_Deps.State_Map.GetGroupMagic(tagEntry.TagGroupIndex);
		tag.FourCC = this->MagicToString(magic);
	}

	// We get the name of this tag.
	tag.TagName = m_Deps.State_Map.GetTagName(index);
	return tag;
}

// Resolves the file offset of a tag's metadata block by its filename.
//
// Each tag in the map has an associated filename (e.g. "objects\characters\spartans\spartans")
// stored in the debug segment. This function searches the tag tabl linearly for a 
// tag whose resolved name matches. 'info.TagName', then converts its compressed 
// memory address into a file offset via Expand() + ToFileOffset().
//
// The returned offset points to the start of the tag's metadata block in the tag
// segment, which is where all of the tag's fields (blocks, references, data) are 
// laid out according to its group's structure definition.
//
// Returns -1 if the tag is not found or the TagInfo is marked invalid.
// For repeated lookups or performance-sensitive paths, prefer GetTagMetaOffsetByIndex().
int64_t System_MapReader::GetTagMetaOffset(const ResolvedTag& tag) const
{
	if (!tag.IsValid) return -1;

	int32_t count = (int32_t)m_Deps.State_Map.GetTagsSize();
	for (int32_t i = 0; i < count; ++i)
	{
		if (m_Deps.State_Map.GetTagName(i) == tag.TagName)
		{
			const Map_TagTableEntry& entry = m_Deps.State_Map.GetTag(i);
			return this->ToFileOffset(this->Expand(
				static_cast<uint32_t>(entry.MemoryAddress)));
		}
	}

	return -1;
}

// Resolves the file offset of a tag's metadata block by its index in the tag table.
//
// This is the direct counterpart of GetTagMetaOffset(). Instead of searching by
// name, it accesses the tag table at a know index, the same index encoded in
// datum handles and tag references throughout the map.
//
// A datum index is a 32-bit handle where the lower 16 bit are the tag table index
// and the upper 16 bits are the salt used to detect stale references. This function
// expects the index already extracted (lower 16 bits only).
//
// Returns -1 if the index is out of range or the tag has no memory address (null tag).
int64_t System_MapReader::GetTagMetaOffsetByIndex(int32_t tagIndex) const
{
	if (tagIndex < 0 ||
		tagIndex >= (int32_t)m_Deps.State_Map.GetTagsSize())
	{
		return -1;
	}

	const Map_TagTableEntry& entry = m_Deps.State_Map.GetTag(tagIndex);
	if (entry.MemoryAddress == 0) return -1;

	return this->ToFileOffset(this->Expand(
		static_cast<uint32_t>(entry.MemoryAddress)));
}

// --- Shared Parsing Infrastructure ---

FILE* System_MapReader::OpenMapFile() const
{
	std::string filePath = m_Deps.State_Map.GetMapFilePath();
	if (filePath.empty()) return nullptr;

	FILE* file = nullptr;
#ifdef _WIN32
	fopen_s(&file, filePath.c_str(), "rb");
#else
	file = fopen(filePath.c_str(), "rb");
#endif

	return file;
}

// Reads a tag reference from the file and returns the corresponding entry 
// from the Tag Table. A tag reference (Map_TagRef) is a 0x10 byte structure 
// embedded in the metadata.
//
// The DatumIndex field (located using offsetof) is a 32-bit handle that identifies
// another tag in the map. The lower 16 bits represent the actual index in the tag table.
//
// This function:
//     1. Positions itself at the DatumIndex field within the reference structure.
//     2. Extracts the index from the tag table (mask 0xFFFF).
//     3. Validates that the index is within the range of the table loaded into memory.
//     4. Returns the complete Map_TagTableEntry object for further processing.
// 
// Returns an empty structure (Map_TagTableEntry{}) if the reference is null (0 or -1),
// if the file pointer is invalid or if the index is out of range.
Map_TagTableEntry System_MapReader::ReadTagRef(FILE* file, int64_t tagRefOffset) const
{
	if (tagRefOffset < 0) return {};

	if (_fseeki64(file, tagRefOffset + offsetof(Map_TagRef, DatumIndex), 
		SEEK_SET) != 0) return {};

	uint32_t datumIndex = 0;
	if (fread(&datumIndex, sizeof(datumIndex), 1, file) != 1) return {};
	if (datumIndex == 0xFFFFFFFF || datumIndex == 0) return {};

	int32_t tagIndex = datumIndex & 0xFFFF;

	if (tagIndex < 0 || tagIndex >=
		(int32_t)m_Deps.State_Map.GetTagsSize()) return {};

	return m_Deps.State_Map.GetTag(tagIndex);
}

// Resolves the file offset of a specific tag's metadata block.
//
// This function takes an entry from the tag table and performs the necessary 
// chain of transformations to navigate the disk:
//     1. Verifies that the tag has a valid memory address.
//     2. Applies Expand() to decompress the 32-bit virtual address.
//     3. Applies ToFileOffset() to convert that virtual address into a file offset.
//
// Returns the file offset (int64_t) where the tag's metadata begins, or -1 if it fails.
int64_t System_MapReader::ResolveTagOffset(const Map_TagTableEntry& tag) const
{
	if (tag.MemoryAddress == 0) return -1;

	return this->ToFileOffset(this->Expand(tag.MemoryAddress));
}

// Reads the header of a Tag Block and stores it in a local data structure.
//
// A Tag Block (Map_TagBlock) is the fundamental mechanism for nested data and
// variable-length arrays in .map files. The 0xC byte structure contains:
//     - EntryCount: Number of elements in the array.
//     - Pointer: Compressed virtual address to the first element.
//
// Unlike the previous version, this function reads the entire structure
// at once into local memory, allowing you to inspect the element count
// before attempting to resolve pointers.
//
// Returns a Map_TagBlock object with the read data, or an empty structure if fseek fails.
Map_TagBlock System_MapReader::ReadTagBlock(FILE* file, int64_t blockHeaderOffset) const
{
	Map_TagBlock block{};
	if (blockHeaderOffset < 0) return block;

	if (_fseeki64(file, blockHeaderOffset, SEEK_SET) == 0)
	{
		fread(&block, sizeof(Map_TagBlock), 1, file);
	}

	return block;
}

// Resolves the file offset for the first element of a Tag Block.
// Takes the previously read block header and validates if it contains data.
//
// If the block is valid (EntryCount > 0), applies the transformation string:
//     Expand(block.Pointer) -> ToFileOffset().
//
// The resulting offset points to the first struct of the array on disk. To read
// subsequent elements, the caller must apply the stride (element size).
//
// Returns the file offset (int64_t) or -1 if the block is empty or invalid.
int64_t System_MapReader::ResolveBlockOffset(const Map_TagBlock& block) const
{
	if (block.EntryCount <= 0 || block.Pointer == 0) return -1;
	return this->ToFileOffset(this->Expand(block.Pointer));
}

std::vector<uint8_t> System_MapReader::ReadResourceData(
	int64_t fileOffset, int32_t size) const
{
	std::vector<uint8_t> buffer(size);
	FILE* file = this->OpenMapFile();
	if (!file) return {};

	_fseeki64(file, fileOffset, SEEK_SET);
	fread(buffer.data(), 1, size, file);
	fclose(file);
	return buffer;
}

std::vector<uint8_t> System_MapReader::ReadResourceDataDecompressed(
	int64_t fileOffset, int32_t compressedSize, int32_t decompressedSize,
	int32_t segmentOffset, int32_t segmentLength) const
{
	if (compressedSize == decompressedSize)
	{
		return ReadResourceData(segmentOffset, segmentLength);
	}

	auto compressed = ReadResourceData(fileOffset, compressedSize);
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

	int32_t length = (std::min)(segmentLength, decompressedSize - segmentOffset);
	return std::vector<uint8_t>(
		decompressed.begin() + segmentOffset,
		decompressed.begin() + segmentOffset + length);
}

// ----------------------------------------------------------------------------
// Primitiva: abre CUALQUIER archivo y lee 'size' bytes desde 'fileOffset'.
// ----------------------------------------------------------------------------
std::vector<uint8_t> System_MapReader::ReadDataFrom(
	const std::string& filePath, int64_t fileOffset, int32_t size) const
{
	if (filePath.empty() || size <= 0) return {};

	FILE* file = nullptr;
#ifdef _WIN32
	fopen_s(&file, filePath.c_str(), "rb");
#else
	file = fopen(filePath.c_str(), "rb");
#endif
	if (!file) return {};

	std::vector<uint8_t> buffer(size);
	_fseeki64(file, fileOffset, SEEK_SET);
	size_t read = fread(buffer.data(), 1, (size_t)size, file);
	fclose(file);

	if (read < (size_t)size) buffer.resize(read);
	return buffer;
}

// ----------------------------------------------------------------------------
// Resuelve "maps\shared.map" -> path absoluto (shared/campaign; "" si no soportado).
// ----------------------------------------------------------------------------
std::string System_MapReader::ResolveExternalCachePath(const std::string& mapPath) const
{
	std::string lower;
	lower.reserve(mapPath.size());
	for (char c : mapPath) lower.push_back((char)std::tolower((unsigned char)c));

	if (lower.find("shared") != std::string::npos)
		return m_Deps.State_Map.GetSharedFilePath();

	if (lower.find("campaign") != std::string::npos)
		return m_Deps.State_Map.GetCampaignFilePath();

	// english.map / mainmenu.map u otros: no soportados.
	return {};
}

// ----------------------------------------------------------------------------
// Igual que ReadResourceDataDecompressed pero desde un .map externo.
// ----------------------------------------------------------------------------
std::vector<uint8_t> System_MapReader::ReadResourceDataDecompressedFrom(
	const std::string& filePath,
	int64_t fileOffset, int32_t compressedSize, int32_t decompressedSize,
	int32_t segmentOffset, int32_t segmentLength) const
{
	if (decompressedSize <= 0 || segmentOffset < 0) return {};

	// Sin comprimir: leer directo el segmento (fileOffset + segmentOffset).
	if (compressedSize == decompressedSize)
	{
		int32_t length = (std::min)(segmentLength, decompressedSize - segmentOffset);
		if (length <= 0) return {};
		return ReadDataFrom(filePath, fileOffset + segmentOffset, length);
	}

	auto compressed = ReadDataFrom(filePath, fileOffset, compressedSize);
	if ((int32_t)compressed.size() < compressedSize) return {};

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

	int32_t length = (std::min)(segmentLength, decompressedSize - segmentOffset);
	if (length <= 0) return {};
	return std::vector<uint8_t>(
		decompressed.begin() + segmentOffset,
		decompressed.begin() + segmentOffset + length);
}

// --- Address Arithmetic ---

// Expands a compressed 32-bit tag memory address into a full virtual address.
//
// In this map format, tag memory addresses stored in TagEntry::MemoryAddress are
// not raw virtual addresses, they are compressed by right-shifting the actual
// address by 2 bits (i.e. dividing by 4). To recover the original virtual address,
// the value must be shifted back left by 2 bits and then offset by 'm_ExpandMagic',
// which encodes the base of the tag segment in the engine's virtual address space.
//
// Null (0x0) and sentinel (0xFFFFFFFF) values are treated as invalid and return 0.
// The result is passed to ToFileOffset() to obtain a usable file offset.
int64_t System_MapReader::Expand(uint32_t address) const
{
	if (address == 0 || address == 0xFFFFFFFF) return 0;

	// Equivalent to: address * 4 + ExpandMagic.
	return ((int64_t)address << 2) + k_ExpandMagic;
}


// Converts a virtual address from the tag segment into a file offset.
//
// The '.map' file is a memory dump: all pointers stored inside it are virtual
// addresses from the game's address space, not file offsets. To read the data
// they point to, the virtual address must be translated back into a byte position
// in the file using the formula:
//
//     fileOffset = virtualAddress - virtualBase + metaFileOffset
//
// Where 'virtualBase' is where the tag segment was mapped in memory, and
// 'metaFileOffset' is where that same segment begins in the file.
// Use this to locate any tag metadata within the '.map' file
// (e.g. matg, scnr, sbsp and all other tag data blocks).
int64_t System_MapReader::ToFileOffset(int64_t virtualAddress) const
{
	return virtualAddress - (int64_t)m_VirtualBaseAddress + 
		(int64_t)m_MetaFileOffset;
}

// Converts a pointer from the debug segment into a file offset.
//
// The debug segment uses a different addressing scheme from the tag segment.
// Pointers into it are stored as 32-bit values and are translated by adding
// a fixed mask ('m_DebugOffsetMask') derived from the header's offset mask table.
// The result is truncated to 32 bits, which is the valid range for this segment.
//
// Use this to locate file name tables and other debug data within the '.map' file.
// It is the counterpart of ToFileOffset() for the debug segment.
int64_t System_MapReader::ToDebugOffset(int64_t pointer) const
{
	return (int64_t)(uint32_t)((uint32_t)pointer + m_DebugOffsetMask);
}

int64_t System_MapReader::ToResourceOffset(int64_t blockOffset) const
{
	return blockOffset + (int64_t)m_ResourceVirtualAddress
		+ (int32_t)m_ResourceOffsetMask;
}

// TODO: offset masks[1] and [3] (and their corresponding sections) are not yet
// used. Each likely corresponds to an additional file segment (possibly locales
// and resources) that requires its own To...Offset() conversion function.

// --- .MAP Parsing ---

// Reads the map header and computes the derived offsets needed to 
// navigate the file.
//
// The map header (0xA000 bytes at the start of the file) contains all the data
// needed to bootstrap the rest of the load. This function reads it in three passes:
//
// 1. Virtual base address and index header address. The virtual base address is 
//    where the tag segment was mapped in the game's memory. The index header 
//    address is a virtual pointer to the index table header, which must be 
//    converted to a file offset before it can be used. Both are stored as 
//    uint64 but only their lower 32 bits are meaningful in this format, 
//    truncation is intentional.
//
// 2. File table fields (count, offset, size, index offset). These describe 
//    the two debug segment tables used to reconstruct tag filenames:
//    the name offset index and the raw string data blob. They are stored 
//    as int32 values and are passed directly to ToDebugOffset() when needed.
//
// 3. Offset mask table and section table (interop data). The offset mask table 
//    holds per-segment deltas used to convert stored pointers into file offsets. 
//    Mask[0] is the debug segment mask; mask[2] is the tag segment mask. 
//    The section table holds the virtual address and size of each segment. 
//    Section[0] is the debug segment; section[2] is the tag segment.
//    
//    For these, two derived values are computed.
//
//        m_MetaFileOffset: where the tag segment begins in the file. 
//                          (tagVirtualAddress + tagOffsetMask (signed delta).
//
//        m_IndexHeaderFileOffset: file offset of the index table header.
//                                 (ToFileOffset(Lo32(indexHeaderAddress)).
//
// All subsequent reads in this system depend on the values cached by this funcion.
bool System_MapReader::ReadHeader(FILE* file)
{
	namespace MH = MapOffsets::MapHeader;

	fseek(file, MH::k_VirtualBaseAddress, SEEK_SET);
	if (fread(&m_VirtualBaseAddress, sizeof(m_VirtualBaseAddress), 1, file) != 1) return false;

	// Stored as uint64 in the file, but only the lower 32 bits are meaningful
	// for address arithmetic in this map format.
	uint64_t indexHeaderAddress = 0;
	if (fread(&indexHeaderAddress, sizeof(indexHeaderAddress), 1, file) != 1) return false;

	fseek(file, MH::k_FileTableCount, SEEK_SET);
	if (fread(&m_FileTableCount, sizeof(m_FileTableCount), 1, file) != 1) return false;
	if (fread(&m_FileTableOffset, sizeof(m_FileTableOffset), 1, file) != 1) return false;
	if (fread(&m_FileTableSize, sizeof(m_FileTableSize), 1, file) != 1) return false;
	if (fread(&m_FileIndexTableOffset, sizeof(m_FileIndexTableOffset), 1, file) != 1) return false;

	// Offset Masks (interop data).

	uint32_t offsetMasks[MH::OffsetMasks::k_Count] = {};
	fseek(file, MH::OffsetMasks::k_Base, SEEK_SET);
	if (fread(offsetMasks, sizeof(uint32_t), MH::OffsetMasks::k_Count, file) != 
		MH::OffsetMasks::k_Count) return false;

	m_DebugOffsetMask	= offsetMasks[0];
	m_TagOffsetMask		= offsetMasks[2];

	// Section Table.

	Map_Header_Sections sections[MH::Sections::k_Count] = {};
	fseek(file, MH::Sections::k_Base, SEEK_SET);
	if (fread(sections, sizeof(Map_Header_Sections), MH::Sections::k_Count, file) !=
		MH::Sections::k_Count) return false;

	m_DebugVirtualAddress	= sections[0].VirtualAddress;
	m_TagVirtualAddress		= sections[2].VirtualAddress;

	// Derived Offsets.
	// MetaFileOffset: where the tag section begins in the file.
	// The tag offset mask is a signed delta applied to the virtual address.
	m_MetaFileOffset = (int64_t)m_TagVirtualAddress + (int32_t)m_TagOffsetMask;

	// Virtual addresses in this format are 32-bit. Truncation is intentional.
	auto Lo32 = [](uint64_t addr) -> int64_t { return (int64_t)(uint32_t)addr; };
	m_IndexHeaderFileOffset = ToFileOffset(Lo32(indexHeaderAddress));

	m_ResourceVirtualAddress = sections[1].VirtualAddress;
	m_ResourceOffsetMask = offsetMasks[1];

	return true;
}

// Reads the index table from the '.map' file.
//
// The index table header is th root of the tag system. It is not located at a
// fixed offset in the file, its position is a virtual address stored in the map
// header, resolved at load time into 'm_IndexHeaderFileOffset'.
//
// The header contains the count and virtual address of each table:
//    - Tag group table: defines the class hierarchy (e.g. "bipd" extends "unit").
//    - Tag table: one entry per tag, holds its group index and memory address.
//
// A magic value is validated first to confirm the file is not corrupt and that
// the address arithmetic resolved to the correct location.
bool System_MapReader::ReadIndexHeader(FILE* file)
{
	namespace ITH = MapOffsets::IndexTableHeader;

	auto seek = [&](size_t fieldOffset) {
		return fseek(file, (long)(m_IndexHeaderFileOffset + fieldOffset), SEEK_SET);
	};

	// Validate magic first.
	uint32_t magic = 0;
	seek(ITH::k_Magic);
	if (fread(&magic, sizeof(magic), 1, file) != 1) return false;
	if (magic != MapMagics::k_IndexHeaderMagic) return false;

	int32_t groupCount = 0;
	seek(ITH::k_NumberOfTagGroups);
	if (fread(&groupCount, sizeof(groupCount), 1, file) != 1) return false;

	uint64_t groupTableAddress = 0;
	seek(ITH::k_TagGroupTableAddress);
	if (fread(&groupTableAddress, sizeof(groupTableAddress), 1, file) != 1) return false;

	int32_t tagCount = 0;
	seek(ITH::k_NumberOfTags);
	if (fread(&tagCount, sizeof(tagCount), 1, file) != 1) return false;

	uint64_t tagTableAddress = 0;
	seek(ITH::k_TagTableAddress);
	if (fread(&tagTableAddress, sizeof(tagTableAddress), 1, file) != 1) return false;

	if (!ReadTagGroups(file, ToFileOffset((int64_t)groupTableAddress), groupCount)) return false;
	if (!ReadTags(file, ToFileOffset((int64_t)tagTableAddress), tagCount)) return false;

	return true;
}

// Reads the tag group table from the '.map' file into memory.
//
// Tag groups define the class system of the engine. Every tag belongs to a group
// (e.g. "bipd" for bipeds, "weap" for weapons), and each group can optionally
// inherit from a parent and grandparent group, forming a three-level hierarchy.
// Each entry is identified by a 4-byte magic (a FourCC, e.g. 0x62697064 = "bipd").
//
// The table is a flat contiguous array in the tag segment. Its address is a 
// virtual pointer resolved via ToFileOffset() before reading.
//
// The entire Table is read in a single fread call into a pre-allocated vector,
// since TagGroupEntry matches the binary layout of each entry exactly.
bool System_MapReader::ReadTagGroups(FILE* file, int64_t fileOffset, int32_t count)
{
	if (count <= 0) return false;

	m_Deps.State_Map.ResizeGroups(count);
	fseek(file, (long)fileOffset, SEEK_SET);
	if (fread(m_Deps.State_Map.GetGroupsData(), sizeof(Map_TagTableGroupEntry),
		count, file) != (size_t)count) return false;

	return true;
}

// Reads the tag table from the '.map' file into memory.
//
// The tag table is the master index of every tag instance in the map. Each entry
// stores three things:
//     - Group index: which tag group (class) this tag belongs to.
//     - Datum Salt: a generation counter used to detect stale handles.
//     - Memory Address: a 32-bit virtual pointer to th tag's metadata block
//       in the tag segment. This is the value passed to Expand() and then
//       ToFileOffset() to locate the tag's data.
//
// A tag's metadata (its actual data fields) is not stored here, this table only
// holds the pointer to it. To read a tag's fields, resolve its memory address
// through Expand() + ToFileOffset() and seek to that position.
//
// The entire table is read in a single fread call into a pre-allocated vector,
// since TagEntry matches the binary layout of each entry exactly.
bool System_MapReader::ReadTags(FILE* file, int64_t fileOffset, int32_t count)
{
	if (count <= 0) return false;

	m_Deps.State_Map.ResizeTags(count);
	fseek(file, (long)fileOffset, SEEK_SET);
	if (fread(m_Deps.State_Map.GetTagsData(), sizeof(Map_TagTableEntry),
		count, file) != (size_t)count) return false;

	return true;
}

// Reads the file names table from the '.map' file into memory.
//
// Tag filenames are stored in the debug segment, which is a separate region of 
// the file from the tag segment. Two tables are needed to reconstruct them:
//
// 1. Name offset table: one int32 per tag, giving the byte offset of that
//    tag's filename within the name data blob. An offset of -1 means the 
//    tag has no filename. Indices here correspond 1:1 with the tag table.
// 
// 2. Name data blob: a flat buffer of concatenated null-terminated ASCII
//    strings. A filename is read by seeking to nameData + nameOffsets[i];
//
// Both tables are addressed via ToDebugOffset(), which applies the debug segment's
// offset mask to convert stored pointers into file offsets. This is the only 
// function in this system that reads from the debug segment.
bool System_MapReader::ReadFileNames(FILE* f) const
{
	if (m_FileTableCount <= 0) return true;

	uint32_t nameIndexOffset = (uint32_t)ToDebugOffset(m_FileIndexTableOffset);
	uint32_t nameDataOffset = (uint32_t)ToDebugOffset(m_FileTableOffset);

	m_Deps.State_Map.ResizeNameOffsets(m_FileTableCount);
	fseek(f, nameIndexOffset, SEEK_SET);
	if (fread(m_Deps.State_Map.GetNameOffsetsData(), sizeof(int32_t),
		m_FileTableCount, f) != (size_t)m_FileTableCount) return false;

	m_Deps.State_Map.ResizeNameData(m_FileTableSize);
	fseek(f, nameDataOffset, SEEK_SET);
	if (fread(m_Deps.State_Map.GetNameData(), sizeof(char),
		m_FileTableSize, f) != (size_t)m_FileTableSize) return false;

	return true;
}

std::string System_MapReader::MagicToString(int32_t magic) const
{
	char str[5]{};

	// Received magic: necs, ihev, jorp, paew, etcetera...

	str[0] = (magic >> 24) & 0xFF;	// s
	str[1] = (magic >> 16) & 0xFF;	// c
	str[2] = (magic >> 8) & 0xFF;	// e
	str[3] = (magic) & 0xFF;		// n

	// Null terminator '\0'.
	str[4] = 0;

	// Expected magic: scen, vehi, proj, weap, bipd, etcetera...
	// (scenery, vehicle, projectile, weapon, biped)

	return std::string(str);
}