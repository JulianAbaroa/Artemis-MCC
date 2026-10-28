export module Engine.Control.Layer;

import Service.Layer;
import Engine.Base.Layer;
import Platform.Input.State;
import Platform.Input.Hook;
import Platform.Simulation.State;
import Platform.Simulation.System;
import Platform.Simulation.Hook;

export namespace Engine::Control
{
    // The control of the engine: the virtual input, the actions of the players and the bots.
    // note: Owner only: it has no callbacks and installs nothing. The one that composes it connects the action source of
    // PlayerActionDetour (D243), the lifecycle, and installs the hooks.
    class Layer
    {
    private:
        using VirtualInputStore = Platform::Input::State::VirtualInputStore;
        using PlayerActionStore = Platform::Input::State::PlayerActionStore;
        using InputUpdateDetour = Platform::Input::Hook::InputUpdateDetour;
        using PlayerActionDetour = Platform::Input::Hook::PlayerActionDetour;

        using BotStore = Platform::Simulation::State::BotStore;
        using BotService = Platform::Simulation::System::BotService;
        using SetMachinesDetour = Platform::Simulation::Hook::SetMachinesDetour;

    public:
        Layer(Service::Layer& service, Engine::Base::Layer& base) :
            m_BotService(service.m_Log, base.m_AOBService, m_BotStore),
            m_InputUpdateDetour(service.m_Log, base.m_HostService, m_VirtualInputStore),
            m_PlayerActionDetour(service.m_Log, base.m_AOBService, m_PlayerActionStore, m_BotService),
            m_SetMachinesDetour(service.m_Log, base.m_AOBService, m_BotStore) {}
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        VirtualInputStore m_VirtualInputStore{};
        PlayerActionStore m_PlayerActionStore{};
        BotStore m_BotStore{};

        // --- System ---
        BotService m_BotService;

        // --- Hook ---
        InputUpdateDetour m_InputUpdateDetour;
        PlayerActionDetour m_PlayerActionDetour;
        SetMachinesDetour m_SetMachinesDetour;
    };
}