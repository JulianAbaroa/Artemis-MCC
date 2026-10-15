export module Export.Tick.Type;

import Tables.Object.Type;
import Tables.Player.Type;
import Tables.Interaction.Type;
import Relations.Classifier.Type;
import Relations.ObjectGraph.Type;
import Relations.PlayerGraph.Type;
import Environment.Details.Type;
import Environment.Entities.Type;
import Environment.Fixtures.Type;
import Environment.Aim.Type;
import Egocentric.Affordance.Type;
import Egocentric.Self.Type;
import Egocentric.Raycast.Type;
import Resolved.Definitions.Type;
import std;

export namespace Export::Tick::Type
{
	using AliveObject = Tables::Object::Type::Alive::Object;
	using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
	using AlivePlayer = Tables::Player::Type::Alive::AlivePlayer;
	using PlayerTable = Tables::Player::Type::Alive::PlayerTable;
	using AliveInteraction = Tables::Interaction::Type::Alive::AliveInteraction;

	using Classified = Relations::Classifier::Type::Classified;
	using Classifieds = std::vector<Classified>;
	using ObjectNode = Relations::ObjectGraph::Type::ObjectNode;
	using ObjectGraph = std::unordered_map<uint32_t, ObjectNode>;
	using PlayerTree = Relations::PlayerGraph::Type::PlayerTree;
	using PlayerGraph = std::vector<PlayerTree>;

	using Entity = Environment::Entities::Type::Entity;
	using Entities = Environment::Entities::Type::Entities;
	using VitalityDetail = Environment::Details::Type::VitalityDetail;
	using VitalityDetails = Environment::Details::Type::VitalityDetails;
	using ZoneDetail = Environment::Details::Type::ZoneDetail;
	using ZoneDetails = Environment::Details::Type::ZoneDetails;
	using TeleporterDetail = Environment::Details::Type::TeleporterDetail;
	using TeleporterDetails = Environment::Details::Type::TeleporterDetails;
	using LiftDetail = Environment::Details::Type::LiftDetail;
	using LiftDetails = Environment::Details::Type::LiftDetails;
	using BarrierDetail = Environment::Details::Type::BarrierDetail;
	using BarrierDetails = Environment::Details::Type::BarrierDetails;
	using Fixtures = Environment::Fixtures::Type::Fixtures;
	using Aim = Environment::Aim::Type::Aim;
	using Aims = std::unordered_map<std::uint32_t, Aim>;

	using Affordance = Egocentric::Affordance::Type::Affordance;
	using Affordances = std::vector<Affordance>;
	using Self = Egocentric::Self::Type::Self;
	using Raycasts = Egocentric::Raycast::Type::Raycasts;
	using MapSbsps = std::unordered_map<std::string, Resolved::Definitions::Type::Sbsp::Sbsp>;
	using MapScnrs = std::unordered_map<std::string, Resolved::Definitions::Type::Scnr::Scnr>;
	using MapSddts = std::unordered_map<std::string, Resolved::Definitions::Type::Sddt::Sddt>;

	struct Tick
	{
		std::uint64_t Generation = 0;

		std::shared_ptr<const MapSbsps> Map;
		std::shared_ptr<const MapScnrs> Limits;
		std::shared_ptr<const MapSddts> Designs;

		// --- Layer 3: Tables ---
		std::shared_ptr<const ObjectTable> ObjectTable;
		std::shared_ptr<const PlayerTable> PlayerTable;
		std::shared_ptr<const AliveInteraction> Interaction;

		// --- Layer 4: Relations ---
		std::shared_ptr<const Classifieds> Classifieds;
		std::shared_ptr<const ObjectGraph> ObjectGraph;
		std::shared_ptr<const PlayerGraph> PlayerGraph;

		// --- Layer 5: Environment ---
		std::shared_ptr<const Entities> Entities;
		std::shared_ptr<const VitalityDetails> VitalityDetails;
		std::shared_ptr<const ZoneDetails> ZoneDetails;
		std::shared_ptr<const TeleporterDetails> TeleporterDetails;
		std::shared_ptr<const LiftDetails> LiftDetails;
		std::shared_ptr<const BarrierDetails> BarrierDetails;
		std::shared_ptr<const Fixtures> Fixtures;
		std::shared_ptr<const Aims> Aims;

		// --- Layer 6: Egocentric ---
		std::shared_ptr<const Self> Self;
		std::shared_ptr<const Affordances> Affordances;
		std::shared_ptr<const Raycasts> Raycasts;
	};
}