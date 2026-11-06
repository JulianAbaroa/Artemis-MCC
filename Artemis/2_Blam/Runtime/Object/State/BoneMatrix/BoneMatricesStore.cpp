module Blam.Runtime.Object.State;
import :BoneMatrices;

import Blam.Runtime.Object.Type;
import std;

namespace
{
    using Blam::Runtime::Object::Type::Constant::k_StorePruneSlack;
}

namespace Blam::Runtime::Object::State
{
    auto BoneMatricesStore::Get(std::uint32_t handle) const -> const BoneMatrixTable*
    {
        auto it = m_BoneMatrixTables.find(handle);
        return it != m_BoneMatrixTables.end() && it->second.Generation == m_Generation ? &it->second.Table : nullptr;
    }

    auto BoneMatricesStore::Write(std::uint32_t handle) -> BoneMatrixTable&
    {
        BoneMatrixEntry& entry{ m_BoneMatrixTables[handle] };
        if (entry.Generation != m_Generation)
        {
            entry.Generation = m_Generation;
            ++m_Written;
        }

        entry.Table.Matrices.clear();
        return entry.Table;
    }

    auto BoneMatricesStore::Discard(std::uint32_t handle) -> void
    {
        auto it = m_BoneMatrixTables.find(handle);
        if (it == m_BoneMatrixTables.end() || it->second.Generation != m_Generation) return;

        it->second.Generation = 0;
        --m_Written;
    }

    auto BoneMatricesStore::Clear() -> void
    {
        if (m_BoneMatrixTables.size() > (2 * m_Written) + k_StorePruneSlack)
        {
            std::erase_if(m_BoneMatrixTables, [&](const auto& pair) { return pair.second.Generation != m_Generation; });
        }

        ++m_Generation;
        m_Written = 0;
    }
}