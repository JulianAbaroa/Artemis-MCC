module DevTools.Viewer.Selection.State;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

namespace DevTools::Viewer::Selection::State
{
    auto SelectionStore::GetSelected() const -> std::uint32_t
    {
        return m_Selected.load(std::memory_order_relaxed);
    }

    auto SelectionStore::SetSelected(std::uint32_t handle) -> void
    {
        m_Selected.store(handle, std::memory_order_relaxed);
        m_SelectedPlayer.store(k_NoHandle, std::memory_order_relaxed);
    }

    auto SelectionStore::Follow(std::uint32_t playerHandle, std::uint32_t bipedHandle) -> void
    {
        m_Selected.store(bipedHandle, std::memory_order_relaxed);
        m_SelectedPlayer.store(playerHandle, std::memory_order_relaxed);
    }

    auto SelectionStore::GetSelectedPlayer() const -> std::uint32_t
    {
        return m_SelectedPlayer.load(std::memory_order_relaxed);
    }

    auto SelectionStore::HasSelection() const -> bool
    {
        return m_Selected.load(std::memory_order_relaxed) != k_NoHandle || m_SelectedPlayer.load(std::memory_order_relaxed) != k_NoHandle;
    }

    auto SelectionStore::Clear() -> void
    {
        m_Selected.store(k_NoHandle, std::memory_order_relaxed);
        m_SelectedPlayer.store(k_NoHandle, std::memory_order_relaxed);
    }

    auto SelectionStore::RequestPick() -> void
    {
        m_IsPickPending.store(true, std::memory_order_relaxed);
    }

    auto SelectionStore::ConsumePick() -> bool
    {
        return m_IsPickPending.exchange(false, std::memory_order_relaxed);
    }

    auto SelectionStore::RequestStep(int delta) -> void
    {
        m_PendingSteps.fetch_add(delta, std::memory_order_relaxed);
    }

    auto SelectionStore::ConsumeStep() -> int
    {
        return m_PendingSteps.exchange(0, std::memory_order_relaxed);
    }
}