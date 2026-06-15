#pragma once

#include "Core/Types/Sources/Tables/Interaction/LiveInteraction.h"

#include <memory>
#include <atomic>

class State_InteractionTable
{
public:
	uintptr_t GetBase() const;
	void SetBase(uintptr_t pointer);

	void Publish(LiveInteraction interaction);
	std::shared_ptr<const LiveInteraction> Acquire() const;

	void Cleanup();

private:
	std::atomic<uintptr_t> m_InteractionTableBase{ 0 };
	std::atomic<std::shared_ptr<const LiveInteraction>> m_pLiveInteraction;
};