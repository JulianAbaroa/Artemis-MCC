export module Engine.Base.Pov.Hook:Probe;

import Common.Log.System;
import Engine.Base.Memory.System;
import Engine.Base.Pov.Type;
import Engine.Base.Pov.State;
import std;

namespace
{
    using Engine::Base::Pov::Type::k_PovProbeHookCount;
}

export namespace Engine::Base::Pov::Hook
{
    // TEMPORAL VW.9a.x (deleted when VW.9a.x closes): hooks that note who moves the view of user 0 and what state changes when the watched player
    // dies. They hook the view assignment 0x53D7C, the unit of a view 0x54070, the unit of a player 0x53CA4, PlayerAssignUnit 0x45814, the
    // cursor candidate 0x2D2018 (the update of a UI screen) and the three creators of UI screens 0x2C9EFC, 0x2C9F88 and 0x2CA014. Each one calls
    // the original and, while PovProbeStore::IsActive, notes its arguments and its caller (the creators also the screen they return and the
    // return addresses of their stack). Nothing changes in the game: the only engine memory read is the u32 behind the pointer argument of the cursor
    // candidate and the fields of the screen it updates (guarded), none is written, and no engine function is called by the probe.
    class PovProbeDetours
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Engine::Base::Memory::System::AOBService;

        using PovProbeWideFunction = Engine::Base::Pov::Type::PovProbeWideFunction;

        using PovProbeStore = Engine::Base::Pov::State::PovProbeStore;

    public:
        PovProbeDetours(Log& log, AOBService& aobService, PovProbeStore& probeStore) :
            m_Log(log), m_AOBService(aobService), m_Store(probeStore) {}
        ~PovProbeDetours() = default;

        // Finds each function by its RVA and exact prologue (window 0) and hooks it. A signature that is not found is logged as an ERROR once and
        // that hook is not installed: the others still are. Does nothing if already installed.
        // return: True if at least one function is hooked.
        // note: The functions are code of the game module, loaded again for every match: install them with the engine, remove them with the unhook.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hooks. Does nothing if not installed. Takes the same mutex as Install, so the
        // unhook of the game thread and the shutdown of the main thread can both call it.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        PovProbeStore& m_Store;

        static inline std::atomic<PovProbeDetours*> s_Instance{ nullptr };
        static inline std::atomic<int> s_InFlight{ 0 };
        static inline PovProbeWideFunction s_ViewPlayerOriginal{ nullptr };
        static inline PovProbeWideFunction s_ViewUnitOriginal{ nullptr };
        static inline PovProbeWideFunction s_PlayerUnitOriginal{ nullptr };
        static inline PovProbeWideFunction s_PlayerAssignUnitOriginal{ nullptr };
        static inline PovProbeWideFunction s_CursorOriginal{ nullptr };
        static inline PovProbeWideFunction s_ScreenCreateOriginal{ nullptr };
        static inline PovProbeWideFunction s_ScreenCreateAltOriginal{ nullptr };
        static inline PovProbeWideFunction s_ScreenCreateExOriginal{ nullptr };

        std::array<void*, k_PovProbeHookCount> m_Addresses{};
        std::atomic<bool> m_IsInstalled{ false };
        std::mutex m_Mutex{};

        static auto __fastcall HookedViewPlayer(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
            std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t;
        static auto __fastcall HookedViewUnit(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
            std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t;
        static auto __fastcall HookedPlayerUnit(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
            std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t;
        static auto __fastcall HookedPlayerAssignUnit(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
            std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t;
        static auto __fastcall HookedScreenCreate(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
            std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t;
        static auto __fastcall HookedScreenCreateAlt(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
            std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t;
        static auto __fastcall HookedScreenCreateEx(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
            std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t;
        static auto __fastcall HookedCursor(std::uint64_t thisPointer, std::uint64_t valuePointer, std::uint64_t a2, std::uint64_t a3,
            std::uint64_t a4, std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t;
    };
}