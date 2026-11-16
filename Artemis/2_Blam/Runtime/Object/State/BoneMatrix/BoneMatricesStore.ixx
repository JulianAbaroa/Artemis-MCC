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

        // return: The table of the handle, or null if it has none. Valid until the next Write or Discard of that handle, or the next Prune.
        auto Get(std::uint32_t handle) const -> const BoneMatrixTable*;

        // Starts the table of the handle, replacing the one it had.
        // return: The table, empty but with the buffers of the last write of that handle, for the caller to fill in place.
        // note: Discard it if the caller ends up with nothing to store. AI thread only.
        auto Write(std::uint32_t handle) -> BoneMatrixTable&;

        // Takes back the table of the handle: Get gives null until the next Write. The buffers stay.
        auto Discard(std::uint32_t handle) -> void;

        // Drops the buffers of the handles with no table when they outnumber the tables twice over (the store would otherwise grow with
        // every object the map ever had).
        auto Prune() -> void;

    private:
        BoneMatrixTables m_BoneMatrixTables{};
        // Entries that are visible.
        std::size_t m_Visible{};
    };
}