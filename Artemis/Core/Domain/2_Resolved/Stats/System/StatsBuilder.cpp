module Resolved.Stats.System;

import Map.Reader.Type;

namespace
{
    namespace Magic = Map::Reader::Type::Magic;

    using TagTableEntry = Map::Reader::Type::Structure::TagTable::Entry;
    using ResolvedProj = Resolved::Stats::Type::Proj::Proj;
    using ResolvedVehi = Resolved::Stats::Type::Vehi::Vehi;
    using ResolvedWeap = Resolved::Stats::Type::Weap::Weap;
}

namespace Resolved::Stats::System
{
    auto StatsBuilder::BuildForMap() -> void
    {
        std::int32_t proj{}, vehi{}, weap{};

        const std::int32_t tagCount = static_cast<std::int32_t>(
            m_TagIndexStore.GetTagsSize());

        for (std::int32_t i = 0; i < tagCount; ++i)
        {
            const TagTableEntry& entry = m_TagIndexStore.GetTag(i);
            if (entry.TagGroupIndex < 0) continue;

            const std::string tagName = m_TagIndexStore.GetTagName(i);
            if (tagName.empty()) continue;

            const std::uint32_t magic = m_TagIndexStore.GetGroupMagic(
                entry.TagGroupIndex);

            if (magic == Magic::Tag::k_Proj)
            {
                if (!this->BuildProj(tagName)) continue;
                ++proj;
            }
            else if (magic == Magic::Tag::k_Vehi)
            {
                if (!this->BuildVehi(tagName)) continue;
                ++vehi;
            }
            else if (magic == Magic::Tag::k_Weap)
            {
                if (!this->BuildWeap(tagName)) continue;
                ++weap;
            }
        }

        m_StatsStore.Freeze();

        m_LogsService.Message("[StatsBuilder] INFO: Stats built."
            " Proj: {} | Vehi: {} | Weap: {}", proj, vehi, weap);
    }

    auto StatsBuilder::BuildProj(const std::string& tagName) -> bool
    {
        const ProjObject* proj = m_ProjStore.Get(tagName);
        if (!proj)
        {
            m_LogsService.Message("[StatsBuilder] WARNING:"
                " Proj tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedProj data = m_ProjBuilder.Build(*proj);
        m_StatsStore.AddResolvedProj(tagName, std::move(data));
        return true;
    }

    auto StatsBuilder::BuildVehi(const std::string& tagName) -> bool
    {
        const VehiObject* vehi = m_VehiStore.Get(tagName);
        if (!vehi)
        {
            m_LogsService.Message("[StatsBuilder] WARNING:"
                " Vehi tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedVehi data = m_VehiBuilder.Build(*vehi);
        m_StatsStore.AddResolvedVehi(tagName, std::move(data));
        return true;
    }

    auto StatsBuilder::BuildWeap(const std::string& tagName) -> bool
    {
        const WeapObject* weap = m_WeapStore.Get(tagName);
        if (!weap)
        {
            m_LogsService.Message("[StatsBuilder] WARNING:"
                " Weap tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedWeap data = m_WeapBuilder.Build(*weap);
        m_StatsStore.AddResolvedWeap(tagName, std::move(data));
        return true;
    }

    auto StatsBuilder::Cleanup() -> void
    {
        m_StatsStore.Cleanup();

        m_LogsService.Message("[StatsBuilder] INFO: Cleanup completed.");
    }
}