#include "pch.h"

#include "State_InteractionTable.h"

uintptr_t State_InteractionTable::GetBase() const 
{ 
	return m_InteractionTableBase.load(); 
}

void State_InteractionTable::SetBase(uintptr_t pointer) 
{ 
	m_InteractionTableBase.store(pointer); 
}

void State_InteractionTable::Publish(LiveInteraction interaction)
{
	auto snap = std::make_shared<const LiveInteraction>(std::move(interaction));
	m_pLiveInteraction.store(snap, std::memory_order_release);
}

std::shared_ptr<const LiveInteraction> State_InteractionTable::Acquire() const
{
	return m_pLiveInteraction.load(std::memory_order_acquire);
}

void State_InteractionTable::Cleanup()
{
	m_InteractionTableBase.store(0);
	m_pLiveInteraction.store(nullptr, std::memory_order_release);
}