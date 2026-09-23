module Map.Reader.System;
import :TagStructure;

namespace
{
	using TagReference = Map::Reader::Type::Structure::Tag::Reference;
}

namespace Map::Reader::System
{
	auto TagStructureService::ReadTagReference(std::ifstream& file,
		std::int64_t tagRefOffset) const -> TagTableEntry
	{
		if (tagRefOffset < 0) return {};

		const auto seekOffset = tagRefOffset + static_cast<std::int64_t>(__builtin_offsetof(TagReference, DatumIndex));
		file.seekg(seekOffset, std::ios::beg);
		if (!file) return {};

		std::uint32_t datumIndex = 0;
		constexpr std::streamsize byteCount = sizeof(datumIndex);
		file.read(reinterpret_cast<char*>(&datumIndex), byteCount);

		if (file.gcount() != byteCount)
		{
			return {};
		}

		if (datumIndex == 0xFFFFFFFF || datumIndex == 0)
		{
			return {};
		}

		const auto tagIndex = static_cast<std::int32_t>(datumIndex & 0xFFFF);

		if (tagIndex < 0 || static_cast<std::size_t>(tagIndex) >= m_TagIndexStore.GetTagsSize())
		{
			return {};
		}

		return m_TagIndexStore.GetTag(tagIndex);
	}

	auto TagStructureService::ReadTagBlock(std::ifstream& file,
		std::int64_t blockHeaderOffset) const -> TagBlock
	{
		TagBlock block{};
		if (blockHeaderOffset < 0) return block;

		file.seekg(blockHeaderOffset, std::ios::beg);
		if (!file) return block;

		constexpr std::streamsize byteCount = sizeof(TagBlock);
		file.read(reinterpret_cast<char*>(&block), byteCount);

		if (file.gcount() != byteCount)
		{
			return {};
		}

		return block;
	}
}