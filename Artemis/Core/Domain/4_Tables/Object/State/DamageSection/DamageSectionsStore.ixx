export module Tables.Object.State:DamageSections;

import Tables.Object.Type;
import std;

export namespace Tables::Object::State
{
    class DamageSectionsStore
    {
    private:
        using DamageSectionTable = Tables::Object::Type::DamageSection::DamageSectionTable;
        using DamageSectionTables = Tables::Object::Type::DamageSection::DamageSectionTables;

    public:
        DamageSectionsStore() = default;
        ~DamageSectionsStore() = default;

        auto Get(std::uint32_t handle) const -> const DamageSectionTable*;
        auto Set(std::uint32_t handle, DamageSectionTable table) -> void;

        auto Clear() -> void;

    private:
        DamageSectionTables m_DamageSectionTables{};
    };
}