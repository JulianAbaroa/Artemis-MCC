export module Tables.Layer;

import Service.Layer;
import Platform.Layer;
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
    class Layer
    {
    private:
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
        Layer(Service::Layer& service, Platform::Layer& platform, Resolved::Layer& resolved) :
            m_ObjectService(service.m_Log, m_ObjectStore, m_BoneMatricesStore, m_DamageSectionsStore, resolved.m_TagResolverService),
            m_PlayerService(service.m_Log, m_PlayerStore),
            m_InteractionService(service.m_Log, m_InteractionStore),
            m_GameService(service.m_Log, m_GameStore)
        {
            auto& lifecycle = platform.m_LifecycleService;

            lifecycle.OnCleanup([this] {
                m_ObjectService.Cleanup();
                m_PlayerService.Cleanup();
                m_InteractionService.Cleanup();
                m_GameService.Cleanup();
            });
        }
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
    };
}