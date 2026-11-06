export module Blam.Runtime.Layer;

import Common.Log.System;
import Blam.MapModel.Layer;
import Blam.Runtime.Object.State;
import Blam.Runtime.Object.System;
import Blam.Runtime.Player.State;
import Blam.Runtime.Player.System;
import Blam.Runtime.Interaction.State;
import Blam.Runtime.Interaction.System;
import Blam.Runtime.Game.State;
import Blam.Runtime.Game.System;

export namespace Blam::Runtime
{
    // Interprets the live objects, players, interaction and game state from the published Snapshot (D190: no process addresses).
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;

        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using BoneMatricesStore = Blam::Runtime::Object::State::BoneMatricesStore;
        using DamageSectionsStore = Blam::Runtime::Object::State::DamageSectionsStore;
        using PlayerTableStore = Blam::Runtime::Player::State::PlayerTableStore;
        using InteractionStore = Blam::Runtime::Interaction::State::InteractionStore;
        using GameStore = Blam::Runtime::Game::State::GameStore;

        using ObjectTableService = Blam::Runtime::Object::System::ObjectTableService;
        using PlayerTableService = Blam::Runtime::Player::System::PlayerTableService;
        using InteractionService = Blam::Runtime::Interaction::System::InteractionService;
        using GameService = Blam::Runtime::Game::System::GameService;

    public:
        Layer(Log& log, Blam::MapModel::Layer& resolved) :
            m_ObjectService(log, m_ObjectStore, m_BoneMatricesStore, m_DamageSectionsStore, resolved.m_TagResolverService),
            m_PlayerService(log, m_PlayerStore),
            m_InteractionService(log, m_InteractionStore),
            m_GameService(log, m_GameStore),
            m_LightObjectService(log, m_LightObjectStore, m_LightBoneMatricesStore, m_LightDamageSectionsStore, resolved.m_TagResolverService),
            m_LightGameService(log, m_LightGameStore) {}
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        ObjectTableStore m_ObjectStore{};
        BoneMatricesStore m_BoneMatricesStore{};
        DamageSectionsStore m_DamageSectionsStore{};
        PlayerTableStore m_PlayerStore{};
        InteractionStore m_InteractionStore{};
        GameStore m_GameStore{};

        // What the light snapshot of a tick without pipeline decodes into (Q101): the weapons and the game globals, apart from the stores of the
        // full tick, which a light tick never touches.
        ObjectTableStore m_LightObjectStore{};
        BoneMatricesStore m_LightBoneMatricesStore{};
        DamageSectionsStore m_LightDamageSectionsStore{};
        GameStore m_LightGameStore{};

        // --- System ---
        ObjectTableService m_ObjectService;
        PlayerTableService m_PlayerService;
        InteractionService m_InteractionService;
        GameService m_GameService;
        ObjectTableService m_LightObjectService;
        GameService m_LightGameService;

        // Removes the objects, players, interaction and game state.
        auto Cleanup() -> void
        {
            m_ObjectService.Cleanup();
            m_PlayerService.Cleanup();
            m_InteractionService.Cleanup();
            m_GameService.Cleanup();
            m_LightObjectStore.Cleanup();
            m_LightGameStore.Cleanup();
        }
    };
}