module Map.Reader.System;
import :TagResolver;

namespace
{
	namespace MapMagic = Map::Reader::Type::Magic;

	auto GroupBit(std::uint32_t magic) -> std::uint32_t
	{
		if (magic == MapMagic::Tag::k_Bipd) return 1u << 0;
		if (magic == MapMagic::Tag::k_Bloc) return 1u << 1;
		if (magic == MapMagic::Tag::k_Coll) return 1u << 2;
		if (magic == MapMagic::Tag::k_Ctrl) return 1u << 3;
		if (magic == MapMagic::Tag::k_Eqip) return 1u << 4;
		if (magic == MapMagic::Tag::k_Hlmt) return 1u << 5;
		if (magic == MapMagic::Tag::k_Mach) return 1u << 6;
		if (magic == MapMagic::Tag::k_Mode) return 1u << 7;
		if (magic == MapMagic::Tag::k_Phmo) return 1u << 8;
		if (magic == MapMagic::Tag::k_Proj) return 1u << 9;
		if (magic == MapMagic::Tag::k_Scen) return 1u << 10;
		if (magic == MapMagic::Tag::k_Scnr) return 1u << 11;
		if (magic == MapMagic::Tag::k_Vehi) return 1u << 12;
		if (magic == MapMagic::Tag::k_Weap) return 1u << 13;

		return 0;
	}
}

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

	auto TagResolverService::EnsureGroupIndexBuilt() const -> void
	{
		std::lock_guard<std::mutex> lock(m_GroupIndexMutex);
		if (m_GroupIndexBuilt) return;

		const std::int32_t tagCount = static_cast<std::int32_t>(
			m_TagIndexStore.GetTagsSize());

		for (std::int32_t i = 0; i < tagCount; ++i)
		{
			const TagTableEntry& entry = m_TagIndexStore.GetTag(i);
			if (entry.TagGroupIndex < 0) continue;

			const std::string tagName = m_TagIndexStore.GetTagName(i);
			if (tagName.empty()) continue;

			const std::uint32_t magic = m_TagIndexStore.
				GetGroupMagic(entry.TagGroupIndex);

			const std::uint32_t bit = GroupBit(magic);
			if (bit == 0) continue;

			m_GroupMaskByName[tagName] |= bit;
		}

		m_GroupIndexBuilt = true;
	}

	auto TagResolverService::HasGroup(
		const std::string& tagName, std::uint32_t magic) const -> bool
	{
		this->EnsureGroupIndexBuilt();

		auto it = m_GroupMaskByName.find(tagName);
		if (it == m_GroupMaskByName.end()) return false;

		return (it->second & GroupBit(magic)) != 0;
	}

	auto TagResolverService::HasBipd(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Bipd);
	}

	auto TagResolverService::HasBloc(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Bloc);
	}

	auto TagResolverService::HasColl(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Coll);
	}

	auto TagResolverService::HasCtrl(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Ctrl);
	}

	auto TagResolverService::HasEqip(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Eqip);
	}

	auto TagResolverService::HasHlmt(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Hlmt);
	}

	auto TagResolverService::HasMach(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Mach);
	}

	auto TagResolverService::HasMode(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Mode);
	}

	auto TagResolverService::HasPhmo(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Phmo);
	}

	auto TagResolverService::HasProj(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Proj);
	}

	auto TagResolverService::HasScen(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Scen);
	}

	auto TagResolverService::HasScnr(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Scnr);
	}

	auto TagResolverService::HasVehi(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Vehi);
	}

	auto TagResolverService::HasWeap(const std::string& tagName) const -> bool
	{
		return this->HasGroup(tagName, MapMagic::Tag::k_Weap);
	}
}