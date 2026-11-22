export module Engine.Control.Layer;

import Services.Layer;
import Engine.Base.Layer;
import Engine.Control.Input.State;
import Engine.Control.Input.System;
import Engine.Control.Input.Hook;
import Engine.Control.Simulation.State;
import Engine.Control.Simulation.System;
import Engine.Control.Simulation.Hook;

export namespace Engine::Control
{
    // The control of the engine: the virtual input, the actions of the players and the bots.
    // note: Owner only: it has no callbacks and installs nothing. The one that composes it connects the action source of
    // PlayerActionDetour (D243), the lifecycle, and installs the hooks.
    class Layer
    {
    private:
        using VirtualInputStore = Engine::Control::Input::State::VirtualInputStore;
        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;
        using PossessionStore = Engine::Control::Input::State::PossessionStore;
        using PossessionService = Engine::Control::Input::System::PossessionService;
        using InputUpdateDetour = Engine::Control::Input::Hook::InputUpdateDetour;
        using ClientOracleDetour = Engine::Control::Input::Hook::ClientOracleDetour;
        using PlayerActionDetour = Engine::Control::Input::Hook::PlayerActionDetour;

        using BotStore = Engine::Control::Simulation::State::BotStore;
        using BotService = Engine::Control::Simulation::System::BotService;
        using SetMachinesDetour = Engine::Control::Simulation::Hook::SetMachinesDetour;

    public:
        Layer(Services::Layer& service, Engine::Base::Layer& base) :
            m_BotService(service.m_Log, base.m_AOBService, m_BotStore, m_PlayerActionStore),
            m_PossessionService(service.m_Log, m_BotService, m_PossessionStore),
            m_InputUpdateDetour(service.m_Log, base.m_HostService, m_VirtualInputStore),
            m_PlayerActionDetour(service.m_Log, base.m_AOBService, base.m_MatchSetupService, m_PlayerActionStore, m_BotService),
            m_ClientOracleDetour(service.m_Log, base.m_AOBService, m_PlayerActionStore, m_BotService),
            m_SetMachinesDetour(service.m_Log, base.m_AOBService, m_BotStore) {}
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        VirtualInputStore m_VirtualInputStore{};
        PlayerActionStore m_PlayerActionStore{};
        BotStore m_BotStore{};

        // The bot the Viewer asks to possess (the render thread writes it, the game thread reads it) and the client state of the possessed bot
        // (VW.9c.2 B). The request ages out like the one of Watch POV (100 ms), but it is a store of this project: Engine.Control does not import Pov.
        Engine::Control::Input::State::PossessRequestStore m_PossessRequestStore{};
        PossessionStore m_PossessionStore{};

        // --- System ---
        BotService m_BotService;
        PossessionService m_PossessionService;

        // --- Hook ---
        InputUpdateDetour m_InputUpdateDetour;
        PlayerActionDetour m_PlayerActionDetour;
        ClientOracleDetour m_ClientOracleDetour;
        SetMachinesDetour m_SetMachinesDetour;
    };
}