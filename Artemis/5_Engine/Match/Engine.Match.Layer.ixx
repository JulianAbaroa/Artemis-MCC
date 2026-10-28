export module Engine.Match.Layer;

import Service.Layer;
import Engine.Base.Layer;
import Platform.Match.State;
import Platform.Match.System;
import Platform.Match.Hook;

export namespace Engine::Match
{
    // The match of the engine: the configuration of the next match and the cycle of the game that the bridge reads.
    // note: Owner only: it has no callbacks and installs nothing. The one that composes it connects the lifecycle and installs the hooks.
    class Layer
    {
    private:
        using MatchConfigStore = Platform::Match::State::MatchConfigStore;
        using MatchCycleStore = Platform::Match::State::MatchCycleStore;
        using MatchConfigService = Platform::Match::System::MatchConfigService;
        using EngineInterfaceDetour = Platform::Match::Hook::EngineInterfaceDetour;
        using LifecycleUpdateDetour = Platform::Match::Hook::LifecycleUpdateDetour;

    public:
        Layer(Service::Layer& service, Engine::Base::Layer& base) :
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