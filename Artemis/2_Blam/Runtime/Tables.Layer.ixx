export module Tables.Layer;

import Common.Log.System;
import Resolved.Layer;
import Tables.Object.State;
import Tables.Object.System;
import Tables.Player.State;
import Tables.Player.System;
import Tables.Interaction.State;
import Tables.Interaction.System;
import Tables.Game.State;
import Tables.Game.System;

export namespace Tables
{
    // Interprets the live objects, players, interaction and game state from the published Snapshot (D190: no process addresses).
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;

        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using BoneMatricesStore = Tables::Object::State::BoneMatricesStore;
        using DamageSectionsStore = Tables::Object::State::DamageSectionsStore;
        using PlayerTableStore = Tables::Player::State::PlayerTableStore;
        using InteractionStore = Tables::Interaction::State::InteractionStore;
        using GameStore = Tables::Game::State::GameStore;

        using ObjectTableService = Tables::Object::System::ObjectTableService;
        using PlayerTableService = Tables::Player::System::PlayerTableService;
        using InteractionService = Tables::Interaction::System::InteractionService;
        using GameService = Tables::Game::System::GameService;

    public:
        Layer(Log& log, Resolved::Layer& resolved) :
            m_ObjectService(log, m_ObjectStore, m_BoneMatricesStore, m_DamageSectionsStore, resolved.m_TagResolverService),
            m_PlayerService(log, m_PlayerStore),
            m_InteractionService(log, m_InteractionStore),
            m_GameService(log, m_GameStore) {}
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

        // --- System ---
        ObjectTableService m_ObjectService;
        PlayerTableService m_PlayerService;
        InteractionService m_InteractionService;
        GameService m_GameService;

        // Removes the objects, players, interaction and game state.
        auto Cleanup() -> void
        {
            m_ObjectService.Cleanup();
            m_PlayerService.Cleanup();
            m_InteractionService.Cleanup();
            m_GameService.Cleanup();
        }
    };
}