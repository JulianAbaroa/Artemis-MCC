export module Tables.Interaction.State;

import Tables.Interaction.Type;
import std;

export namespace Tables::Interaction::State
{
	class InteractionStore
	{
	private:
		using AliveInteraction = Tables::Interaction::Type::Alive::Interaction;

	public:
		InteractionStore() = default;
		~InteractionStore() = default;

		auto GetBase() const -> std::uintptr_t;
		auto SetBase(std::uintptr_t pointer) -> void;

		auto Publish(AliveInteraction interaction) -> void;
		auto Acquire() const -> std::shared_ptr<const AliveInteraction>;

		auto Cleanup() -> void;

	private:
		std::atomic<std::uintptr_t> m_InteractionTableBase{ 0 };
		std::atomic<std::shared_ptr<const AliveInteraction>> m_pAliveInteraction{};
	};
}