export module Engine.Base.Memory.Hook:ObserverCamera;

import Common.Log.System;
import Engine.Base.Memory.Type;
import Engine.Base.Memory.State;
import Engine.Base.Memory.System;
import std;

export namespace Engine::Base::Memory::Hook
{
    // Hooks the function that builds the camera result of a user (VW.5b). With an override published and for user 0, it writes the pose into
    // the WORK of the observer before the call, so the engine computes the position, the location (culling) and the vertical field of view
    // the normal way, and puts back what it wrote after the call. A pose older than k_OverrideMaxAge is ignored. Faults caught while touching the
    // engine are counted in the store.
    class ObserverCameraDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using Signature = Engine::Base::Memory::Type::Signature::Signature;

        using ObserverCameraFunction = Engine::Base::Memory::Type::ObserverCamera::ObserverCameraFunction;

        using CameraOverrideStore = Engine::Base::Memory::State::CameraOverrideStore;

        using AOBService = Engine::Base::Memory::System::AOBService;

    public:
        ObserverCameraDetour(Log& log, AOBService& aobService, CameraOverrideStore& overrideStore) :
            m_Log(log), m_AOBService(aobService), m_OverrideStore(overrideStore) {}
        ~ObserverCameraDetour() = default;

        // Finds the function by its RVA and prologue and hooks it. A prologue that does not match, or no TLS slot of the thread context, is logged
        // as an ERROR and nothing is hooked. Does nothing if already installed. Can be called again after Uninstall.
        // return: True if the function is hooked.
        // note: The function is code of the game module, loaded again for every match: install it with the engine, remove it with the unhook.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        // note: Can be called from two threads (the unhook of the game thread and the shutdown of the main thread): Install and Uninstall take one
        // mutex, so the second caller finds the hook already gone. The statics the detour reads are cleared only after the hook is removed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        CameraOverrideStore& m_OverrideStore;

        static inline std::atomic<ObserverCameraDetour*> s_Instance{ nullptr };
        static inline std::atomic<int> s_InFlight{ 0 };
        static inline ObserverCameraFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsInstalled{ false };
        std::atomic<const std::uint32_t*> m_EngineTlsIndex{ nullptr };
        std::mutex m_Mutex{};

        static auto __fastcall HookedObserverCamera(std::uint32_t user, void* out) -> void;
    };
}