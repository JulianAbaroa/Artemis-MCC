module Blam.Runtime.Object.State;
import :DamageSections;

import Blam.Runtime.Object.Type;
import std;

namespace
{
    using Blam::Runtime::Object::Type::Constant::k_StorePruneSlack;
}

namespace Blam::Runtime::Object::State
{
    auto DamageSectionsStore::Get(std::uint32_t handle) const -> const DamageSectionTable*
    {
        auto it = m_DamageSectionTables.find(handle);
        return it != m_DamageSectionTables.end() && it->second.IsVisible ? &it->second.Table : nullptr;
    }

    auto DamageSectionsStore::Write(std::uint32_t handle) -> DamageSectionTable&
    {
        DamageSectionEntry& entry{ m_DamageSectionTables[handle] };
        if (!entry.IsVisible)
        {
            entry.IsVisible = true;
            ++m_Visible;
        }

        entry.Table.Sections.clear();
        entry.Table.Regions.Size = 0;
        entry.Table.Regions.Offset = 0;
        entry.Table.Regions.Bytes.clear();
        return entry.Table;
    }

    auto DamageSectionsStore::Discard(std::uint32_t handle) -> void
    {
        auto it = m_DamageSectionTables.find(handle);
        if (it == m_DamageSectionTables.end() || !it->second.IsVisible) return;

        it->second.IsVisible = false;
        --m_Visible;
    }

    auto DamageSectionsStore::Prune() -> void
    {
        if (m_DamageSectionTables.size() > (2 * m_Visible) + k_StorePruneSlack)
        {
            std::erase_if(m_DamageSectionTables, [](const auto& pair) { return !pair.second.IsVisible; });
        }
    }
}