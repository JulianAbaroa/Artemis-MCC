module Map.Reader.System;
import :TagResolver;

namespace Map::Reader::System
{
	auto TagResolverService::ResolveHandle(std::uint32_t handle) const -> TagInfo
	{
		if (!m_FileStore.IsLoaded())
		{
			m_LogsService.Message("[TagResolverService] WARNING: No map loaded.");
			return {};
		}

		std::uint16_t salt = (handle >> 16) & 0xFFFF;
		std::uint16_t index = handle & 0xFFFF;

		if (salt == 0xFFFF || index == 0xFFFF)
		{
			m_LogsService.Message("[TagResolverService] WARNING: Salt or Index are invalid.");
			return {};
		}

		if (index >= m_TagIndexStore.GetTagsSize())
		{
			m_LogsService.Message("[TagResolverService] WARNING: Index {} is out of range.", index);
			return {};
		}

		const TagTableEntry& tagEntry = m_TagIndexStore.GetTag(index);
		if (tagEntry.DatumIndexSalt == 0xFFFF)
		{
			m_LogsService.Message("[TagResolverService] WARNING: DatumSalt is invalid.");
			return {};
		}

		if (tagEntry.DatumIndexSalt != salt)
		{
			m_LogsService.Message("[TagResolverService] WARNING:"
				" DatumSalt mismatch. Expected: {}, Got: {}",
				salt, tagEntry.DatumIndexSalt);
			return {};
		}

		TagInfo tagInfo;
		tagInfo.IsValid = true;

		if (tagEntry.TagGroupIndex >= 0 && tagEntry.TagGroupIndex <
		(std::uint16_t)m_TagIndexStore.GetGroupsSize())
		{
			std::uint32_t magic = m_TagIndexStore.
				GetGroupMagic(tagEntry.TagGroupIndex);

			tagInfo.FourCC = this->MagicToString(magic);
		}

		tagInfo.TagName = m_TagIndexStore.GetTagName(index);
		return tagInfo;
	}

	auto TagResolverService::GetTagOffset(std::int32_t tagIndex) const -> std::int64_t
	{
		if (tagIndex < 0 || tagIndex >= 
			(std::int32_t)m_TagIndexStore.GetTagsSize())
		{
			return -1;
		}

		return this->ResolveTagOffset(m_TagIndexStore.GetTag(tagIndex));
	}

	auto TagResolverService::ResolveTagOffset(
		const TagTableEntry& tag) const -> std::int64_t
	{
		if (tag.MemoryAddress == 0) return -1;

		return m_FormulaService.ToFileOffset(m_FormulaService.Expand(tag.MemoryAddress));
	}

	auto TagResolverService::ResolveTagReferenceName(
		const TagReference& ref) const -> std::string
	{
		if (ref.DatumIndex == 0xFFFFFFFF || ref.DatumIndex == 0)
		{
			return {};
		}

		std::int32_t tagIndex = ref.DatumIndex & 0xFFFF;
		if (tagIndex < 0 || tagIndex >= 
			(std::int32_t)m_TagIndexStore.GetTagsSize())
		{
			return {};
		}

		return m_TagIndexStore.GetTagName(tagIndex);
	}

	auto TagResolverService::ResolveBlockOffset(
		const TagBlock& block) const -> std::int64_t
	{
		if (block.EntryCount <= 0 || block.Pointer == 0) return -1;

		return m_FormulaService.ToFileOffset(m_FormulaService.Expand(block.Pointer));
	}

	auto TagResolverService::MagicToString(std::int32_t magic) const -> std::string
	{
		char str[5]{};

		str[0] = (magic >> 24) & 0xFF;
		str[1] = (magic >> 16) & 0xFF;
		str[2] = (magic >> 8) & 0xFF;
		str[3] = (magic) & 0xFF;
		str[4] = 0;

		return std::string(str);
	}
}