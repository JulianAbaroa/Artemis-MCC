export module Engine.Base.Layer;

import Services.Layer;
import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.System;
import Engine.Base.Lifecycle.Hook;
import Engine.Base.Host.State;
import Engine.Base.Host.System;
import Engine.Base.Host.Hook;
import Engine.Base.Memory.System;

export namespace Engine::Base
{
    // The base of the engine: lifecycle of the game, host (MCC) local users and session checks, signature search and memory reads.
    // note: Owner only: it has no callbacks and installs nothing. The one that composes it connects the lifecycle and installs the hooks.
    class Layer
    {
    private:
        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using LifecycleService = Engine::Base::Lifecycle::System::LifecycleService;
        using EngineInitializeDetour = Engine::Base::Lifecycle::Hook::EngineInitializeDetour;
        using DestroySubsystemsDetour = Engine::Base::Lifecycle::Hook::DestroySubsystemsDetour;
        using OpenMapDetour = Engine::Base::Lifecycle::Hook::OpenMapDetour;

        using LocalUserStore = Engine::Base::Host::State::LocalUserStore;
        using HostService = Engine::Base::Host::System::HostService;
        using LocalUserGetPlayerDetour = Engine::Base::Host::Hook::LocalUserGetPlayerDetour;
        using LocalUserGetProfileDetour = Engine::Base::Host::Hook::LocalUserGetProfileDetour;
        using LocalUserGetGamepadMappingDetour = Engine::Base::Host::Hook::LocalUserGetGamepadMappingDetour;
        using CanAcceptAnyJoinRequestDetour = Engine::Base::Host::Hook::CanAcceptAnyJoinRequestDetour;
        using SessionJoinCheckPatch = Engine::Base::Host::Hook::SessionJoinCheckPatch;

        using AOBService = Engine::Base::Memory::System::AOBService;
        using MemoryReaderService = Engine::Base::Memory::System::MemoryReaderService;

    public:
        explicit Layer(Services::Layer& service) :
            m_LifecycleService(service.m_Log, m_LifecycleStore),
            m_AOBService(service.m_Log),
            m_HostService(service.m_Log, m_AOBService),
            m_EngineInitializeDetour(service.m_Log, m_AOBService, m_LifecycleStore, m_LifecycleService),
            m_DestroySubsystemsDetour(service.m_Log, m_AOBService, m_LifecycleStore, m_LifecycleService),
            m_OpenMapDetour(service.m_Log, m_AOBService, m_LifecycleService),
            m_LocalUserGetPlayerDetour(service.m_Log, m_HostService, m_LocalUserStore),
            m_LocalUserGetProfileDetour(service.m_Log, m_HostService, m_LocalUserStore),
            m_LocalUserGetGamepadMappingDetour(service.m_Log, m_HostService, m_LocalUserStore),
            m_CanAcceptAnyJoinRequestDetour(service.m_Log, m_AOBService, m_LocalUserStore),
            m_SessionJoinCheckPatch(service.m_Log, m_AOBService, m_LocalUserStore) {}
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        LifecycleStore m_LifecycleStore{};
        LocalUserStore m_LocalUserStore{};

        // --- System ---
        LifecycleService m_LifecycleService;
        AOBService m_AOBService;
        HostService m_HostService;
        MemoryReaderService m_MemoryReaderService{};

        // --- Hook ---
        EngineInitializeDetour m_EngineInitializeDetour;
        DestroySubsystemsDetour m_DestroySubsystemsDetour;
        OpenMapDetour m_OpenMapDetour;
        LocalUserGetPlayerDetour m_LocalUserGetPlayerDetour;
        LocalUserGetProfileDetour m_LocalUserGetProfileDetour;
        LocalUserGetGamepadMappingDetour m_LocalUserGetGamepadMappingDetour;
        CanAcceptAnyJoinRequestDetour m_CanAcceptAnyJoinRequestDetour;
        SessionJoinCheckPatch m_SessionJoinCheckPatch;
    };
}