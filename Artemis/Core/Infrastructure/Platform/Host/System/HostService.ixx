export module Platform.Host.System;

import Service.Logs.System;
import Platform.Memory.System;
import std;

export namespace Platform::Host::System
{
    // Finds the game manager of MCC and the functions of its vtable.
    // note: The game manager is the interface MCC gives the game DLL (input, local users, profiles, network). haloreach.dll calls it
    // through the vtable, so a detour on the MCC function catches every call of the engine.
    class HostService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using AOBService = Platform::Memory::System::AOBService;

    public:
        HostService(LogsService& logsService, AOBService& aobService) :
            m_LogsService(logsService), m_AOBService(aobService) {}
        ~HostService() = default;

        HostService(const HostService&) = delete;
        auto operator=(const HostService&) -> HostService& = delete;

        // Returns the address of a function of the game manager vtable.
        // param index: Entry of the vtable (k_*Index of Platform.Host.Type).
        // return: nullptr if the game manager is not found. The cause is logged.
        // note: Resolves the game manager on the first call and keeps it for the whole process.
        auto GetFunctionAddress(std::size_t index) -> void*;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;

        std::atomic<void**> m_VTable{ nullptr };

        // Finds the game manager object by signature and reads its vtable.
        // return: nullptr if the signature is not found or the object has no vtable yet.
        auto ResolveVTable() -> void**;
    };
}