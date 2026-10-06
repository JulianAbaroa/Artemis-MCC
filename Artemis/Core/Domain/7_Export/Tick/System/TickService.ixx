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
import Environment.Aim.State;
import Egocentric.Self.State;
import Egocentric.Affordance.State;
import Egocentric.Raycast.State;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Export.Tick.Type;
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
		using AimStore = Environment::Aim::State::AimStore;
		using SelfStore = Egocentric::Self::State::SelfStore;
		using AffordanceStore = Egocentric::Affordance::State::AffordanceStore;
		using RaycastStore = Egocentric::Raycast::State::RaycastStore;
		using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
		using TickStore = Export::Tick::State::TickStore;
		using MapSbsps = Export::Tick::Type::MapSbsps;

	public:
		TickService(ObjectTableStore& objectStore, PlayerTableStore& playerStore,
			InteractionStore& interactionStore, ClassifierStore& classifierStore,
			ObjectGraphStore& objectGraphStore, PlayerGraphStore& playerGraphStore,
			CollidableStore& collidableStore, FixturesStore& fixturesStore,
			HealthStore& healthStore, AimStore& aimStore, SelfStore& selfStore,
			AffordanceStore& affordanceStore, RaycastStore& raycastStore,
			DefinitionsStore& definitionsStore, TickStore& tickStore) :
			m_ObjectStore(objectStore), m_PlayerStore(playerStore),
			m_InteractionStore(interactionStore), m_ClassifierStore(classifierStore),
			m_ObjectGraphStore(objectGraphStore), m_PlayerGraphStore(playerGraphStore),
			m_CollidableStore(collidableStore), m_FixturesStore(fixturesStore),
			m_HealthStore(healthStore), m_AimStore(aimStore), m_SelfStore(selfStore),
			m_AffordanceStore(affordanceStore), m_RaycastStore(raycastStore),
			m_DefinitionsStore(definitionsStore), m_TickStore(tickStore) {}
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
		AimStore& m_AimStore;
		SelfStore& m_SelfStore;
		AffordanceStore& m_AffordanceStore;
		RaycastStore& m_RaycastStore;
		DefinitionsStore& m_DefinitionsStore;
		TickStore& m_TickStore;
		std::shared_ptr<const MapSbsps> m_Map{};
	};
}