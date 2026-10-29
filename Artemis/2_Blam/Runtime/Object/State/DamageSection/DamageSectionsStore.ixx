export module Blam.Runtime.Object.State:DamageSections;

import Blam.Runtime.Object.Type;
import std;

export namespace Blam::Runtime::Object::State
{
    class DamageSectionsStore
    {
    private:
        using DamageSectionTable = Blam::Runtime::Object::Type::DamageSection::DamageSectionTable;
        using DamageSectionTables = Blam::Runtime::Object::Type::DamageSection::DamageSectionTables;

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