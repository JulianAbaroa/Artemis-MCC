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
        return it != m_DamageSectionTables.end() && it->second.Generation == m_Generation ? &it->second.Table : nullptr;
    }

    auto DamageSectionsStore::Write(std::uint32_t handle) -> DamageSectionTable&
    {
        DamageSectionEntry& entry{ m_DamageSectionTables[handle] };
        if (entry.Generation != m_Generation)
        {
            entry.Generation = m_Generation;
            ++m_Written;
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
        if (it == m_DamageSectionTables.end() || it->second.Generation != m_Generation) return;

        it->second.Generation = 0;
        --m_Written;
    }

    auto DamageSectionsStore::Clear() -> void
    {
        if (m_DamageSectionTables.size() > (2 * m_Written) + k_StorePruneSlack)
        {
            std::erase_if(m_DamageSectionTables, [&](const auto& pair) { return pair.second.Generation != m_Generation; });
        }

        ++m_Generation;
        m_Written = 0;
    }
}