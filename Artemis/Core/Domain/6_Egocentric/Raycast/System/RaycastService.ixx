export module Egocentric.Raycast.System;

import Service.Logs.System;
import Common.Math.Type;
import Tables.Player.Type;
import Tables.Player.State;
import Tables.Object.Type;
import Tables.Object.State;
import Resolved.World.State;
import Resolved.World.System;
import Resolved.Stats.Type;
import Resolved.Stats.State;
import Environment.Collidable.Type;
import Environment.Collidable.System;
import Relations.PlayerGraph.Type;
import Relations.PlayerGraph.State;
import Egocentric.Self.Type;
import Egocentric.Self.State;
import Egocentric.Raycast.Type;
import Egocentric.Raycast.State;
import std;

export namespace Egocentric::Raycast::System
{
	class RaycastService
	{
	private:
		using Vec3 = Common::Math::Type::Vec3;
		using Triangle = Common::Math::Type::Triangle;
		using AlivePlayer = Tables::Player::Type::Alive::Player;
		using PlayerTable = std::unordered_map<std::uint32_t, AlivePlayer>;
		using AliveObject = Tables::Object::Type::Alive::Object;
		using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
		using ResolvedWeap = Resolved::Stats::Type::Weap::Weap;
		using PlayerTree = Relations::PlayerGraph::Type::PlayerTree;
		using PlayerGraph = std::vector<PlayerTree>;
		using Collidable = Environment::Collidable::Type::Collidable;
		using CachedDynamic = Egocentric::Raycast::Type::CachedDynamic;
		using DynamicCache = std::unordered_map<std::uint32_t, CachedDynamic>;
		using Self = Egocentric::Self::Type::Self;
		using Raycasts = Egocentric::Raycast::Type::Raycasts;
		using RaycastHit = Egocentric::Raycast::Type::RaycastHit;
		using SelfExclusion = std::unordered_set<std::uint32_t>;
		using PerceptionOrigin = Egocentric::Raycast::Type::PerceptionOrigin;

		using LogsService = Service::Logs::System::LogsService;
		using PlayerTableStore = Tables::Player::State::PlayerTableStore;
		using ObjectTableStore = Tables::Object::State::ObjectTableStore;
		using WorldStore = Resolved::World::State::WorldStore;
		using StatsStore = Resolved::Stats::State::StatsStore;
		using SbspRaycaster = Resolved::World::System::SbspRaycaster;
		using PlayerGraphStore = Relations::PlayerGraph::State::PlayerGraphStore;
		using CollidableService = Environment::Collidable::System::CollidableService;
		using SelfStore = Egocentric::Self::State::SelfStore;
		using RaycastStore = Egocentric::Raycast::State::RaycastStore;

	public:
		RaycastService(LogsService& logsService, PlayerTableStore& playerStore,
			ObjectTableStore& objectStore, WorldStore& worldStore, StatsStore& statsStore,
			SbspRaycaster& sbspRaycaster, SelfStore& selfStore,
			CollidableService& collidableService, PlayerGraphStore& playerGraphStore,
			RaycastStore& raycastStore) :
			m_LogsService(logsService), m_PlayerStore(playerStore),
			m_ObjectStore(objectStore), m_WorldStore(worldStore), m_StatsStore(statsStore),
			m_SbspRaycaster(sbspRaycaster), m_SelfStore(selfStore),
			m_CollidableService(collidableService), m_PlayerGraphStore(playerGraphStore),
			m_RaycastStore(raycastStore) {
		}
		~RaycastService() = default;

		auto Update() -> void;

		auto Cleanup() -> void;

	private:
		LogsService& m_LogsService;
		PlayerTableStore& m_PlayerStore;
		ObjectTableStore& m_ObjectStore;
		WorldStore& m_WorldStore;
		StatsStore& m_StatsStore;
		SbspRaycaster& m_SbspRaycaster;
		SelfStore& m_SelfStore;
		CollidableService& m_CollidableService;
		PlayerGraphStore& m_PlayerGraphStore;
		RaycastStore& m_RaycastStore;

		auto ResolveAimRange(std::uint32_t selfPlayerHandle) const -> float;
		auto ResolveAimRangeUncapped(std::uint32_t selfPlayerHandle) const -> float;

		auto BuildSelfExclusion(std::uint32_t selfPlayerHandle,
			std::uint32_t selfBipedHandle) const -> SelfExclusion;

		auto ResolvePerceptionOrigin(std::uint32_t selfPlayerHandle,
			std::uint32_t selfBipedHandle) const -> PerceptionOrigin;

		auto BuildDynamicCache(const Vec3& origin, float radius,
			const SelfExclusion& selfExclusion) -> DynamicCache;

		auto CastRay(const Vec3& origin, const Vec3& direction, float maxDistance,
			const DynamicCache& dynamicCache) const -> RaycastHit;
	};
}