#include "pch.h"
#include "Core/States/Domain/Tables/InteractionTableState.h"

uintptr_t InteractionTableState::GetInteractionTableBase() const { return m_InteractionTablebase.load(); }
void InteractionTableState::SetInteractionTableBase(uintptr_t pointer) { m_InteractionTablebase.store(pointer); }

LiveInteraction InteractionTableState::GetLiveInteraction() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_LiveInteraction;
}

void InteractionTableState::SetLiveInteraction(LiveInteraction interaction)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_LiveInteraction = interaction;
}

void InteractionTableState::Cleanup()
{
	m_InteractionTablebase.store(0);

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_LiveInteraction = {};
}