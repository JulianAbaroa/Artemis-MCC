export module Engine.Base.Layer;

import Service.Layer;
import Platform.Lifecycle.State;
import Platform.Lifecycle.System;
import Platform.Lifecycle.Hook;
import Platform.Host.State;
import Platform.Host.System;
import Platform.Host.Hook;
import Platform.Memory.System;

export namespace Engine::Base
{
    // The base of the engine: lifecycle of the game, host (MCC) local users and session checks, signature search and memory reads.
    // note: Owner only: it has no callbacks and installs nothing. The one that composes it connects the lifecycle and installs the hooks.
    class Layer
    {
    private:
        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using LifecycleService = Platform::Lifecycle::System::LifecycleService;
        using EngineInitializeDetour = Platform::Lifecycle::Hook::EngineInitializeDetour;
        using DestroySubsystemsDetour = Platform::Lifecycle::Hook::DestroySubsystemsDetour;
        using OpenMapDetour = Platform::Lifecycle::Hook::OpenMapDetour;

        using LocalUserStore = Platform::Host::State::LocalUserStore;
        using HostService = Platform::Host::System::HostService;
        using LocalUserGetPlayerDetour = Platform::Host::Hook::LocalUserGetPlayerDetour;
        using LocalUserGetProfileDetour = Platform::Host::Hook::LocalUserGetProfileDetour;
        using LocalUserGetGamepadMappingDetour = Platform::Host::Hook::LocalUserGetGamepadMappingDetour;
        using CanAcceptAnyJoinRequestDetour = Platform::Host::Hook::CanAcceptAnyJoinRequestDetour;
        using SessionJoinCheckPatch = Platform::Host::Hook::SessionJoinCheckPatch;

        using AOBService = Platform::Memory::System::AOBService;
        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

    public:
        explicit Layer(Service::Layer& service) :
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