module Tables.Interaction.State;

namespace Tables::Interaction::State
{
	auto InteractionStore::GetBase() const -> std::uintptr_t
	{ 
		return m_InteractionTableBase.load(); 
	}

	auto InteractionStore::SetBase(std::uintptr_t pointer) -> void
	{ 
		m_InteractionTableBase.store(pointer); 
	}

	auto InteractionStore::Publish(AliveInteraction interaction) -> void
	{
		auto snap = std::make_shared<const AliveInteraction>(std::move(interaction));
		m_pAliveInteraction.store(snap, std::memory_order_release);
	}

	auto InteractionStore::Acquire() const -> std::shared_ptr<const AliveInteraction>
	{
		return m_pAliveInteraction.load(std::memory_order_acquire);
	}

	auto InteractionStore::Cleanup() -> void
	{
		m_InteractionTableBase.store(0);
		m_pAliveInteraction.store(nullptr, std::memory_order_release);
	}
}