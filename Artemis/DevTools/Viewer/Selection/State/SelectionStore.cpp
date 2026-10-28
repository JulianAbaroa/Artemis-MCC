module Viewer.Selection.State;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

namespace Viewer::Selection::State
{
    auto SelectionStore::GetSelected() const -> std::uint32_t
    {
        return m_Selected.load(std::memory_order_relaxed);
    }

    auto SelectionStore::SetSelected(std::uint32_t handle) -> void
    {
        m_Selected.store(handle, std::memory_order_relaxed);
    }

    auto SelectionStore::HasSelection() const -> bool
    {
        return m_Selected.load(std::memory_order_relaxed) != k_NoHandle;
    }

    auto SelectionStore::Clear() -> void
    {
        m_Selected.store(k_NoHandle, std::memory_order_relaxed);
    }

    auto SelectionStore::RequestPick() -> void
    {
        m_IsPickPending.store(true, std::memory_order_relaxed);
    }

    auto SelectionStore::ConsumePick() -> bool
    {
        return m_IsPickPending.exchange(false, std::memory_order_relaxed);
    }
}