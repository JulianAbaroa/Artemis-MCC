module Resolved.Tag.System;

import Map.Reader.Type;

namespace
{
    using Map::Reader::Type::Structure::TagTable::TagTableEntry;

    using Map::Reader::Type::Magic::Tag::k_Coll;
    using Map::Reader::Type::Magic::Tag::k_Hlmt;
    using Map::Reader::Type::Magic::Tag::k_Mode;

    // Gets the bit that marks a group in the group mask of a tag name.
    // Only the groups that the object table queries have a bit.
    // return: 0 for any other group.
    auto GroupBit(std::uint32_t magic) -> std::uint32_t
    {
        if (magic == k_Coll) return 1u << 0;
        if (magic == k_Hlmt) return 1u << 1;
        if (magic == k_Mode) return 1u << 2;

        return 0;
    }
}

namespace Resolved::Tag::System
{
    auto TagResolverService::ResolveHandle(std::uint32_t handle) const -> TagInfo
    {
        if (!m_FileStore.IsLoaded())
        {
            m_Log.Message("[TagResolverService] WARNING: No map loaded.");
            return {};
        }

        std::uint16_t salt = (handle >> 16) & 0xFFFF;
        std::uint16_t index = handle & 0xFFFF;

        if (salt == 0xFFFF || index == 0xFFFF)
        {
            m_Log.Message("[TagResolverService] WARNING: Salt or Index are invalid.");
            return {};
        }

        if (index >= m_TagIndexStore.GetTagsSize())
        {
            m_Log.Message("[TagResolverService] WARNING: Index {} is out of range.", index);
            return {};
        }

        const TagTableEntry& tagEntry = m_TagIndexStore.GetTag(index);
        if (tagEntry.DatumIndexSalt == 0xFFFF)
        {
            m_Log.Message("[TagResolverService] WARNING: DatumSalt is invalid.");
            return {};
        }

        if (tagEntry.DatumIndexSalt != salt)
        {
            m_Log.Message("[TagResolverService] WARNING:"
                " DatumSalt mismatch. Expected: {}, Got: {}.",
                salt, tagEntry.DatumIndexSalt);
            return {};
        }

        TagInfo tagInfo{};
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
        if (m_IsGroupIndexBuilt) return;

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

        m_IsGroupIndexBuilt = true;
    }

    auto TagResolverService::Cleanup() -> void
    {
        std::lock_guard<std::mutex> lock(m_GroupIndexMutex);

        m_GroupMaskByName.clear();
        m_IsGroupIndexBuilt = false;
    }

    auto TagResolverService::HasGroup(
        const std::string& tagName, std::uint32_t magic) const -> bool
    {
        this->EnsureGroupIndexBuilt();

        auto it = m_GroupMaskByName.find(tagName);
        if (it == m_GroupMaskByName.end()) return false;

        return (it->second & GroupBit(magic)) != 0;
    }

    auto TagResolverService::HasColl(const std::string& tagName) const -> bool
    {
        return this->HasGroup(tagName, k_Coll);
    }

    auto TagResolverService::HasHlmt(const std::string& tagName) const -> bool
    {
        return this->HasGroup(tagName, k_Hlmt);
    }

    auto TagResolverService::HasMode(const std::string& tagName) const -> bool
    {
        return this->HasGroup(tagName, k_Mode);
    }
}