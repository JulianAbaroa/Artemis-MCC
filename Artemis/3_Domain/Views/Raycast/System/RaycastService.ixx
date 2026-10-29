export module Views.Raycast.System;

import Common.Math.Type;
import Common.Log.System;
import Blam.MapModel.World.System;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Definitions.State;
import Blam.Runtime.Player.Type;
import Blam.Runtime.Player.State;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Perception.Collidable.Type;
import Perception.Collidable.System;
import Perception.Entities.Type;
import Perception.Entities.State;
import Perception.Players.Type;
import Perception.Players.State;
import Views.Self.Type;
import Views.Self.State;
import Views.Raycast.Type;
import Views.Raycast.State;
import std;

export namespace Views::Raycast::System
{
    class RaycastService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using Triangle = Common::Math::Type::Triangle;
        using AlivePlayer = Blam::Runtime::Player::Type::Alive::AlivePlayer;
        using PlayerTable = Blam::Runtime::Player::Type::Alive::PlayerTable;
        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using ObjectTable = Blam::Runtime::Object::Type::Alive::ObjectTable;
        using Weap = Blam::MapModel::Definitions::Type::Weap::Weap;
        using Collidable = Perception::Collidable::Type::Collidable;
        using CachedDynamic = Views::Raycast::Type::CachedDynamic;
        using DynamicCache = Views::Raycast::Type::DynamicCache;
        using Self = Views::Self::Type::Self;
        using Raycasts = Views::Raycast::Type::Raycasts;
        using RaycastHit = Views::Raycast::Type::RaycastHit;
        using SelfExclusion = Views::Raycast::Type::SelfExclusion;
        using PerceptionOrigin = Views::Raycast::Type::PerceptionOrigin;

        using Log = Common::Log::System::Log;
        using PlayerTableStore = Blam::Runtime::Player::State::PlayerTableStore;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using Raycaster = Blam::MapModel::World::System::Raycaster;
        using EntitiesStore = Perception::Entities::State::EntitiesStore;
        using PlayersStore = Perception::Players::State::PlayersStore;
        using CollidableService = Perception::Collidable::System::CollidableService;
        using SelfStore = Views::Self::State::SelfStore;
        using RaycastStore = Views::Raycast::State::RaycastStore;

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