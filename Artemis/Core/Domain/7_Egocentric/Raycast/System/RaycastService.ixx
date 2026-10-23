export module Egocentric.Raycast.System;

import Common.Math.Type;
import Common.Log.System;
import Resolved.World.System;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Tables.Player.Type;
import Tables.Player.State;
import Tables.Object.Type;
import Tables.Object.State;
import Environment.Collidable.Type;
import Environment.Collidable.System;
import Environment.Entities.Type;
import Environment.Entities.State;
import Environment.Players.Type;
import Environment.Players.State;
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
        using AlivePlayer = Tables::Player::Type::Alive::AlivePlayer;
        using PlayerTable = Tables::Player::Type::Alive::PlayerTable;
        using AliveObject = Tables::Object::Type::Alive::AliveObject;
        using ObjectTable = Tables::Object::Type::Alive::ObjectTable;
        using Weap = Resolved::Definitions::Type::Weap::Weap;
        using Collidable = Environment::Collidable::Type::Collidable;
        using CachedDynamic = Egocentric::Raycast::Type::CachedDynamic;
        using DynamicCache = Egocentric::Raycast::Type::DynamicCache;
        using Self = Egocentric::Self::Type::Self;
        using Raycasts = Egocentric::Raycast::Type::Raycasts;
        using RaycastHit = Egocentric::Raycast::Type::RaycastHit;
        using SelfExclusion = Egocentric::Raycast::Type::SelfExclusion;
        using PerceptionOrigin = Egocentric::Raycast::Type::PerceptionOrigin;

        using Log = Common::Log::System::Log;
        using PlayerTableStore = Tables::Player::State::PlayerTableStore;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using Raycaster = Resolved::World::System::Raycaster;
        using EntitiesStore = Environment::Entities::State::EntitiesStore;
        using PlayersStore = Environment::Players::State::PlayersStore;
        using CollidableService = Environment::Collidable::System::CollidableService;
        using SelfStore = Egocentric::Self::State::SelfStore;
        using RaycastStore = Egocentric::Raycast::State::RaycastStore;

    public:
        RaycastService(Log& log, PlayerTableStore& playerStore,
            ObjectTableStore& objectStore, DefinitionsStore& definitionsStore,
            Raycaster& raycaster, SelfStore& selfStore,
            CollidableService& collidableService, EntitiesStore& entitiesStore,
            PlayersStore& playersStore, RaycastStore& raycastStore) :
            m_Log(log), m_PlayerStore(playerStore),
            m_ObjectStore(objectStore), m_DefinitionsStore(definitionsStore),
            m_Raycaster(raycaster), m_SelfStore(selfStore),
            m_CollidableService(collidableService), m_EntitiesStore(entitiesStore),
            m_PlayersStore(playersStore), m_RaycastStore(raycastStore)
        {
        }
        ~RaycastService() = default;

        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        Log& m_Log;
        PlayerTableStore& m_PlayerStore;
        ObjectTableStore& m_ObjectStore;
        DefinitionsStore& m_DefinitionsStore;
        Raycaster& m_Raycaster;
        SelfStore& m_SelfStore;
        CollidableService& m_CollidableService;
        EntitiesStore& m_EntitiesStore;
        PlayersStore& m_PlayersStore;
        RaycastStore& m_RaycastStore;

        auto ResolveAimRange(std::uint32_t selfPlayerHandle) const -> float;
        auto ResolveAimRangeUncapped(std::uint32_t selfPlayerHandle) const -> float;

        // Collects the handles the rays of the agent ignore: its biped, its loadout and the vehicle it rides with its parts.
        auto BuildSelfExclusion(std::uint32_t selfPlayerHandle,
            std::uint32_t selfBipedHandle) const -> SelfExclusion;

        // Returns the center and the forward of the biped, or of its vehicle when it rides one.
        auto ResolvePerceptionOrigin(std::uint32_t selfBipedHandle) const -> PerceptionOrigin;

        auto BuildDynamicCache(const Vec3& origin, float radius,
            const SelfExclusion& selfExclusion) -> DynamicCache;

        auto CastRay(const Vec3& origin, const Vec3& direction, float maxDistance,
            const DynamicCache& dynamicCache) const -> RaycastHit;
    };
}