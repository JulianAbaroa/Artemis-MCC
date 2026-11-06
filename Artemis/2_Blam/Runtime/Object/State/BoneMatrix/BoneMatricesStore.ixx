export module Blam.Runtime.Object.State:BoneMatrices;

import Blam.Runtime.Object.Type;
import std;

export namespace Blam::Runtime::Object::State
{
    class BoneMatricesStore
    {
    private:
        using BoneMatrixTable = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
        using BoneMatrixEntry = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixEntry;
        using BoneMatrixTables = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTables;

    public:
        BoneMatricesStore() = default;
        ~BoneMatricesStore() = default;

        // return: The table written for the handle since the last Clear, or null. Valid until the next Clear or Write of that handle.
        auto Get(std::uint32_t handle) const -> const BoneMatrixTable*;

        // Starts the table of the handle for this tick.
        // return: The table, empty but with the buffers of the last write of that handle, for the caller to fill in place.
        // note: Discard it if the caller ends up with nothing to store. AI thread only.
        auto Write(std::uint32_t handle) -> BoneMatrixTable&;

        // Takes back the table of the handle: Get gives null until the next Write.
        auto Discard(std::uint32_t handle) -> void;

        // Makes every table invisible. The buffers stay for the next tick, except those of the handles that were not written for a
        // long time (the store would otherwise grow with every object the map ever had).
        auto Clear() -> void;

    private:
        BoneMatrixTables m_BoneMatrixTables{};
        // Generation of the tables that are visible. Starts at 1 so a new entry (0) is not.
        std::uint64_t m_Generation{ 1 };
        // Tables written in this generation.
        std::size_t m_Written{};
    };
}