export module Export.Tick.System;

import Tables.Object.State;
import Tables.Player.State;
import Tables.Interaction.State;
import Relations.Classifier.State;
import Relations.ObjectGraph.State;
import Relations.PlayerGraph.State;
import Environment.Collidable.State;
import Environment.Fixtures.State;
import Environment.Health.State;
import Egocentric.Self.State;
import Egocentric.Affordance.State;
import Export.Tick.State;
import std;

export namespace Export::Tick::System
{
	class TickService
	{
	private:
		using ObjectTableStore = Tables::Object::State::ObjectTableStore;
		using PlayerTableStore = Tables::Player::State::PlayerTableStore;
		using InteractionStore = Tables::Interaction::State::InteractionStore;
		using ClassifierStore = Relations::Classifier::State::ClassifierStore;
		using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;
		using PlayerGraphStore = Relations::PlayerGraph::State::PlayerGraphStore;
		using CollidableStore = Environment::Collidable::State::CollidableStore;
		using FixturesStore = Environment::Fixtures::State::FixturesStore;
		using HealthStore = Environment::Health::State::HealthStore;
		using SelfStore = Egocentric::Self::State::SelfStore;
		using AffordanceStore = Egocentric::Affordance::State::AffordanceStore;
		using TickStore = Export::Tick::State::TickStore;

	public:
		TickService(ObjectTableStore& objectStore, PlayerTableStore& playerStore,
			InteractionStore& interactionStore, ClassifierStore& classifierStore,
			ObjectGraphStore& objectGraphStore, PlayerGraphStore& playerGraphStore,
			CollidableStore& collidableStore, FixturesStore& fixturesStore,
			HealthStore& healthStore, SelfStore& selfStore,
			AffordanceStore& affordanceStore, TickStore& tickStore) : 
			m_ObjectStore(objectStore), m_PlayerStore(playerStore),
			m_InteractionStore(interactionStore), m_ClassifierStore(classifierStore),
			m_ObjectGraphStore(objectGraphStore), m_PlayerGraphStore(playerGraphStore),
			m_CollidableStore(collidableStore), m_FixturesStore(fixturesStore),
			m_HealthStore(healthStore), m_SelfStore(selfStore),
			m_AffordanceStore(affordanceStore), m_TickStore(tickStore) {}
		~TickService() = default;

		auto Assemble(std::uint64_t generation) -> void;

	private:
		ObjectTableStore& m_ObjectStore;
		PlayerTableStore& m_PlayerStore;
		InteractionStore& m_InteractionStore;
		ClassifierStore& m_ClassifierStore;
		ObjectGraphStore& m_ObjectGraphStore;
		PlayerGraphStore& m_PlayerGraphStore;
		CollidableStore& m_CollidableStore;
		FixturesStore& m_FixturesStore;
		HealthStore& m_HealthStore;
		SelfStore& m_SelfStore;
		AffordanceStore& m_AffordanceStore;
		TickStore& m_TickStore;
	};
}