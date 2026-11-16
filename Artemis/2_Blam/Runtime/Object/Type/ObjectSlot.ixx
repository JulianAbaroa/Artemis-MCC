export module Blam.Runtime.Object.Type:Slot;

import :Alive;

import Common.Engine.Type;
import std;

namespace
{
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Common::Engine::Type::k_MaxObjects;
    using Common::Engine::Type::k_NoHandle;
}

export namespace Blam::Runtime::Object::Type::Slot
{
    // The live object of one slot of the object table (the handle index, without the salt).
    struct ObjectSlot
    {
        // Handle of the live object, or none while the slot has no row.
        std::uint32_t Handle{ k_NoHandle };
        // The row in the live table. Nodes of the table do not move when it rehashes, so it stays valid while the row lives.
        AliveObject* Object{};
        // Publish tick of the last change of the row: its creation, or a change of any field.
        std::uint64_t ChangedTick{};
    };

    struct ObjectSlots : std::array<ObjectSlot, k_MaxObjects>
    {
    };

    // An object that left the live table and the tick it left in.
    struct DeadObject
    {
        std::uint32_t Handle{};
        std::uint64_t Tick{};
    };

    // A table of the publish pool and the tick it was last brought up to date at (0 = it must be copied whole).
    struct PooledTable
    {
        std::shared_ptr<ObjectTable> Table{};
        std::uint64_t Tick{};
    };
}