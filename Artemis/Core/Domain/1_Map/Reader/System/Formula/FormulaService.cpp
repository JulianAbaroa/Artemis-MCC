module Map.Reader.System;
import :Formula;

namespace
{
	namespace Magic = Map::Reader::Type::Magic;
}

namespace Map::Reader::System
{
	auto FormulaService::Initialize(HeaderInfo headerInfo) -> void
	{
		m_HeaderInfo = headerInfo;
	}

	auto FormulaService::Cleanup() -> void
	{
		m_HeaderInfo = {};
	}

	auto FormulaService::Expand(std::uint32_t address) const -> std::int64_t
	{
		if (address == 0 || address == 0xFFFFFFFF) return 0;
		return ((std::int64_t)address << 2) + Magic::k_Expand;
	}

	auto FormulaService::ToFileOffset(std::int64_t virtualAddress) const -> std::int64_t
	{
		return virtualAddress - (std::int64_t)m_HeaderInfo.VirtualBaseAddress +
			m_HeaderInfo.TagSectionFileOffset;
	}

	auto FormulaService::ToDebugOffset(std::int64_t pointer) const -> std::int64_t
	{
		return pointer + m_HeaderInfo.DebugOffsetMask;
	}

	auto FormulaService::ToResourceOffset(std::int64_t blockOffset) const -> std::int64_t
	{
		return blockOffset + (std::int64_t)m_HeaderInfo.ResourceVirtualAddress +
			(std::int32_t)m_HeaderInfo.ResourceOffsetMask;
	}

	auto FormulaService::ResolveFixupOffset(std::uint16_t address,
		std::uint16_t addressUpperBits, 
		std::uint8_t addressLocationHighBits) const -> std::uint32_t
	{
		constexpr std::uint32_t k_FixupMask = 0x0FFFFFFF;

		std::uint32_t offset = (std::uint32_t)address |
			(std::uint32_t)addressUpperBits << 16 |
			(std::uint32_t)addressLocationHighBits << 24;

		return offset & k_FixupMask;
	}
}