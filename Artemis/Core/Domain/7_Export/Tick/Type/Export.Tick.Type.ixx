export module Export.Tick.Type;

import Tables.Object.Type;
import Tables.Player.Type;
import Tables.Interaction.Type;
import Relations.Classifier.Type;
import Relations.ObjectGraph.Type;
import Relations.PlayerGraph.Type;
import Environment.Collidable.Type;
import Environment.Health.Type;
import Environment.Fixtures.Type;
import Egocentric.Affordance.Type;
import Egocentric.Self.Type;
import std;

export namespace Export::Tick::Type
{
	using AliveObject = Tables::Object::Type::Alive::Object;
	using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
	using AlivePlayer = Tables::Player::Type::Alive::Player;
	using PlayerTable = std::unordered_map<std::uint32_t, AlivePlayer>;
	using Interaction = Tables::Interaction::Type::Alive::Interaction;

	using Classified = Relations::Classifier::Type::Classified;
	using Classifieds = std::vector<Classified>;
	using ObjectNode = Relations::ObjectGraph::Type::ObjectNode;
	using ObjectGraph = std::unordered_map<uint32_t, ObjectNode>;
	using PlayerTree = Relations::PlayerGraph::Type::PlayerTree;
	using PlayerGraph = std::vector<PlayerTree>;

	using Collidable = Environment::Collidable::Type::Collidable;
	using Collidables = std::vector<Collidable>;
	using Health = Environment::Health::Type::Health;
	using Healths = std::unordered_map<std::uint32_t, Health>;
	using Fixtures = Environment::Fixtures::Type::Fixtures;

	using Affordance = Egocentric::Affordance::Type::Affordance;
	using Affordances = std::vector<Affordance>;
	using Self = Egocentric::Self::Type::Self;

	struct Tick
	{
		std::uint64_t Generation = 0;

		// --- Layer 3: Tables ---
		std::shared_ptr<const ObjectTable> ObjectTable;
		std::shared_ptr<const PlayerTable> PlayerTable;
		std::shared_ptr<const Interaction> Interaction;

		// --- Layer 4: Relations ---
		std::shared_ptr<const Classifieds> Classifieds;
		std::shared_ptr<const ObjectGraph> ObjectGraph;
		std::shared_ptr<const PlayerGraph> PlayerGraph;

		// --- Layer 5: Environment ---
		std::shared_ptr<const Collidables> Collidables;
		std::shared_ptr<const Fixtures> Fixtures;
		std::shared_ptr<const Healths> Healths;

		// --- Layer 6: Egocentric ---
		std::shared_ptr<const Self> Self;
		std::shared_ptr<const Affordances> Affordances;
		// std::shared_ptr<const Entities> Entities;
	};
}