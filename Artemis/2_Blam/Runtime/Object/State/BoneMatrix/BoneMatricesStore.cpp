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
        return it != m_BoneMatrixTables.end() && it->second.IsVisible ? &it->second.Table : nullptr;
    }

    auto BoneMatricesStore::Write(std::uint32_t handle) -> BoneMatrixTable&
    {
        BoneMatrixEntry& entry{ m_BoneMatrixTables[handle] };
        if (!entry.IsVisible)
        {
            entry.IsVisible = true;
            ++m_Visible;
        }

        entry.Table.Matrices.clear();
        return entry.Table;
    }

    auto BoneMatricesStore::Discard(std::uint32_t handle) -> void
    {
        auto it = m_BoneMatrixTables.find(handle);
        if (it == m_BoneMatrixTables.end() || !it->second.IsVisible) return;

        it->second.IsVisible = false;
        --m_Visible;
    }

    auto BoneMatricesStore::Prune() -> void
    {
        if (m_BoneMatrixTables.size() > (2 * m_Visible) + k_StorePruneSlack)
        {
            std::erase_if(m_BoneMatrixTables, [](const auto& pair) { return !pair.second.IsVisible; });
        }
    }
}