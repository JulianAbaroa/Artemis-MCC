export module Views.Raycast.System;

export import :Cast;

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
        using DynamicCache = Views::Raycast::Type::DynamicCache;
        using Self = Views::Self::Type::Self;
        using Raycasts = Views::Raycast::Type::Raycasts;
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
    };
}