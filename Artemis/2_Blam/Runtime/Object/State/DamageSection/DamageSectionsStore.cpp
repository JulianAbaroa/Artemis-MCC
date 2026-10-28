module Tables.Object.State;
import :DamageSections;

namespace Tables::Object::State
{
    auto DamageSectionsStore::Get(std::uint32_t handle) const -> const DamageSectionTable*
    {
        auto it = m_DamageSectionTables.find(handle);
        return it != m_DamageSectionTables.end() ? &it->second : nullptr;
    }

    auto DamageSectionsStore::Set(std::uint32_t handle, DamageSectionTable table) -> void
    {
        m_DamageSectionTables.emplace(handle, std::move(table));
    }

    auto DamageSectionsStore::Clear() -> void
    {
        m_DamageSectionTables.clear();
    }
}