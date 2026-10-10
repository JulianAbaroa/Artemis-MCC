export module Tables.Object.State:BoneMatrices;

import Tables.Object.Type;
import std;

export namespace Tables::Object::State
{
    class BoneMatricesStore
    {
    private:
        using BoneMatrixTable = Tables::Object::Type::BoneMatrix::BoneMatrixTable;
        using BoneMatrixTables = std::unordered_map<std::uint32_t, BoneMatrixTable>;

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