export module Blam.Runtime.Object.State:BoneMatrices;

import Blam.Runtime.Object.Type;
import std;

export namespace Blam::Runtime::Object::State
{
    class BoneMatricesStore
    {
    private:
        using BoneMatrixTable = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
        using BoneMatrixTables = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTables;

    public:
        BoneMatricesStore() = default;
        ~BoneMatricesStore() = default;

        auto Get(std::uint32_t handle) const -> const BoneMatrixTable*;
        auto Set(std::uint32_t handle, BoneMatrixTable table) -> void;

        auto Clear() -> void;

    private:
        BoneMatrixTables m_BoneMatrixTables{};
    };
}