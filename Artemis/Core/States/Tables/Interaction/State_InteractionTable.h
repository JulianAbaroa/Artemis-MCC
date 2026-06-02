#pragma once

#include "Core/Types/Interaction/LiveInteraction.h"

#include <atomic>
#include <mutex>

class State_InteractionTable
{
public:
	uintptr_t GetBase() const;
	void SetBase(uintptr_t pointer);

	LiveInteraction GetLiveInteraction() const;
	void SetLiveInteraction(LiveInteraction interaction);

	void Cleanup();

private:
	std::atomic<uintptr_t> m_InteractionTablebase{ 0 };

	LiveInteraction m_LiveInteraction{};
	mutable std::mutex m_Mutex;
};