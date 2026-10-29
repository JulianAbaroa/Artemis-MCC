export module Engine.Match.Layer;

import Services.Layer;
import Engine.Base.Layer;
import Engine.Match.State;
import Engine.Match.System;
import Engine.Match.Hook;

export namespace Engine::Match
{
    // The match of the engine: the configuration of the next match and the cycle of the game that the bridge reads.
    // note: Owner only: it has no callbacks and installs nothing. The one that composes it connects the lifecycle and installs the hooks.
    class Layer
    {
    private:
        using MatchConfigStore = Engine::Match::State::MatchConfigStore;
        using MatchCycleStore = Engine::Match::State::MatchCycleStore;
        using MatchConfigService = Engine::Match::System::MatchConfigService;
        using EngineInterfaceDetour = Engine::Match::Hook::EngineInterfaceDetour;
        using LifecycleUpdateDetour = Engine::Match::Hook::LifecycleUpdateDetour;

    public:
        Layer(Services::Layer& service, Engine::Base::Layer& base) :
            m_MatchConfigService(service.m_Log, service.m_SettingsStore, m_MatchConfigStore),
            m_EngineInterfaceDetour(service.m_Log, base.m_AOBService, base.m_MemoryReaderService, m_MatchCycleStore, m_MatchConfigService),
            m_LifecycleUpdateDetour(service.m_Log, base.m_AOBService, m_MatchConfigStore, m_MatchCycleStore) {}
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        MatchConfigStore m_MatchConfigStore{};
        MatchCycleStore m_MatchCycleStore{};

        // --- System ---
        MatchConfigService m_MatchConfigService;

        // --- Hook ---
        EngineInterfaceDetour m_EngineInterfaceDetour;
        LifecycleUpdateDetour m_LifecycleUpdateDetour;
    };
}