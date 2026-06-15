#include "pch.h"

#include "State_Selection.h"

uint32_t State_Selection::GetSelected() const
{
    return m_Selected.load(std::memory_order_relaxed);
}

void State_Selection::SetSelected(uint32_t handle)
{
    m_Selected.store(handle, std::memory_order_relaxed);
}

void State_Selection::Clear()
{
    m_Selected.store(kNoSelection, std::memory_order_relaxed);
}

bool State_Selection::HasSelection() const
{
    return m_Selected.load(std::memory_order_relaxed) != kNoSelection;
}

void State_Selection::RequestPick()
{
    m_PickPending.store(true, std::memory_order_relaxed);
}

bool State_Selection::ConsumePick()
{
    return m_PickPending.exchange(false, std::memory_order_relaxed);
}