#include "pch.h"
#include "Core/States/Domain/Tables/State_InteractionTable.h"

uintptr_t State_InteractionTable::GetInteractionTableBase() const { return m_InteractionTablebase.load(); }
void State_InteractionTable::SetInteractionTableBase(uintptr_t pointer) { m_InteractionTablebase.store(pointer); }

LiveInteraction State_InteractionTable::GetLiveInteraction() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_LiveInteraction;
}

void State_InteractionTable::SetLiveInteraction(LiveInteraction interaction)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_LiveInteraction = interaction;
}

void State_InteractionTable::Cleanup()
{
	m_InteractionTablebase.store(0);

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_LiveInteraction = {};
}