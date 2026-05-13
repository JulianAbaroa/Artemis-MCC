#pragma once

// Types.
#include "Core/Types/Domain/Interaction/LiveInteraction.h"

#include <atomic>
#include <mutex>

class State_InteractionTable
{
public:
	uintptr_t GetInteractionTableBase() const;
	void SetInteractionTableBase(uintptr_t pointer);

	LiveInteraction GetLiveInteraction() const;
	void SetLiveInteraction(LiveInteraction interaction);

	void Cleanup();

private:
	LiveInteraction m_LiveInteraction{};
	mutable std::mutex m_Mutex;

	std::atomic<uintptr_t> m_InteractionTablebase{ 0 };
};