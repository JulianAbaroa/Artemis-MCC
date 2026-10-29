export module Engine.Base.Lifecycle.Hook:OpenMap;

import Common.Log.System;
import Engine.Base.Memory.System;
import Engine.Base.Lifecycle.Type;
import Engine.Base.Lifecycle.System;
import std;

export namespace Engine::Base::Lifecycle::Hook
{
    // Hooks the engine map opening. Once the original returns, reports the full path of the file the engine opened.
    // note: Only reports it. What the file is and what to do with it is decided by the owner of the callback (D179).
    class OpenMapDetour
    {
    private:
        using OpenMapFunction = Engine::Base::Lifecycle::Type::OpenMapFunction;

        using Log = Common::Log::System::Log;
        using AOBService = Engine::Base::Memory::System::AOBService;
        using LifecycleService = Engine::Base::Lifecycle::System::LifecycleService;

    public:
        OpenMapDetour(Log& log, AOBService& aobService, LifecycleService& lifecycleService) :
            m_Log(log), m_AOBService(aobService), m_LifecycleService(lifecycleService) {}
        ~OpenMapDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        auto Install() -> void;

        // Removes the hook once the calls in flight finish. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        LifecycleService& m_LifecycleService;

        static inline OpenMapDetour* s_Instance{ nullptr };
        static inline OpenMapFunction s_OriginalFunction{ nullptr };
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedOpenMap(std::uint64_t param1,
            std::uint64_t param2, std::uint64_t mapRelativePath,
            std::uint32_t* param4) -> void;
    };
}