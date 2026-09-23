module Tables.Object.State;
import :BoneMatrices;

namespace Tables::Object::State
{
    auto BoneMatricesStore::Get(std::uint32_t handle) const -> const BoneMatrixTable*
    {
        auto it = m_BoneMatrixTables.find(handle);
        return it != m_BoneMatrixTables.end() ? &it->second : nullptr;
    }

    auto BoneMatricesStore::Set(std::uint32_t handle, BoneMatrixTable table) -> void
    {
        m_BoneMatrixTables.emplace(handle, std::move(table));
    }

    auto BoneMatricesStore::Clear() -> void
    { 
        m_BoneMatrixTables.clear(); 
    }
}