export module Blam.Runtime.Object.State:DamageSections;

import Blam.Runtime.Object.Type;
import std;

export namespace Blam::Runtime::Object::State
{
    class DamageSectionsStore
    {
    private:
        using DamageSectionTable = Blam::Runtime::Object::Type::DamageSection::DamageSectionTable;
        using DamageSectionEntry = Blam::Runtime::Object::Type::DamageSection::DamageSectionEntry;
        using DamageSectionTables = Blam::Runtime::Object::Type::DamageSection::DamageSectionTables;

    public:
        DamageSectionsStore() = default;
        ~DamageSectionsStore() = default;

        // return: The table of the handle, or null if it has none. Valid until the next Write or Discard of that handle, or the next Prune.
        auto Get(std::uint32_t handle) const -> const DamageSectionTable*;

        // Starts the table of the handle, replacing the one it had.
        // return: The table, empty (no sections, no region bytes) but with the buffers of the last write of that handle, for the caller to fill in place.
        // note: Discard it if the caller ends up with nothing to store. AI thread only.
        auto Write(std::uint32_t handle) -> DamageSectionTable&;

        // Takes back the table of the handle: Get gives null until the next Write. The buffers stay.
        auto Discard(std::uint32_t handle) -> void;

        // Drops the buffers of the handles with no table when they outnumber the tables twice over (the store would otherwise grow with
        // every object the map ever had).
        auto Prune() -> void;

    private:
        DamageSectionTables m_DamageSectionTables{};
        // Entries that are visible.
        std::size_t m_Visible{};
    };
}