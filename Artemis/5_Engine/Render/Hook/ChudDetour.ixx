export module Engine.Render.Hook:Chud;

import Common.Log.System;
import Engine.Base.Memory.Type;
import Engine.Base.Memory.System;
import Engine.Render.Type;
import Engine.Render.State;
import Engine.Render.System;
import std;

export namespace Engine::Render::Hook
{
    // Hooks the three HUD passes of the game (chud draw, late draw and 3D draw). With the render flag HideGameHud they return without
    // calling the original. The draw also measures the thread, the render target and the viewport it runs with (user 0 only).
    class ChudDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using Signature = Engine::Base::Memory::Type::Signature::Signature;

        using AOBService = Engine::Base::Memory::System::AOBService;

        using ChudDrawFunction = Engine::Render::Type::ChudDrawFunction;

        using RenderStore = Engine::Render::State::RenderStore;

        using RenderService = Engine::Render::System::RenderService;

    public:
        ChudDetour(Log& log, AOBService& aobService, RenderStore& renderStore, RenderService& renderService) :
            m_Log(log), m_AOBService(aobService), m_RenderStore(renderStore), m_RenderService(renderService) {}
        ~ChudDetour() = default;

        // Finds each pass by its RVA and prologue and hooks it. A pass whose prologue does not match is logged as an ERROR and left
        // alone, the others are still hooked. Does nothing if already installed. Can be called again after Uninstall.
        // return: True if the draw of the list (the one that measures) is hooked.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hooks that were installed. Does nothing if not installed.
        // note: Can be called from two threads (the unhook of the game thread and the shutdown of the main thread): Install and Uninstall take one
        // mutex, so the second caller finds the hooks already gone and never clears the originals of a hook that is still live.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        RenderStore& m_RenderStore;
        RenderService& m_RenderService;

        static inline std::atomic<ChudDetour*> s_Instance{ nullptr };
        static inline ChudDrawFunction s_OriginalDraw{ nullptr };
        static inline ChudDrawFunction s_OriginalDrawLate{ nullptr };
        static inline ChudDrawFunction s_OriginalDraw3D{ nullptr };

        std::atomic<void*> m_DrawAddress{ nullptr };
        std::atomic<void*> m_DrawLateAddress{ nullptr };
        std::atomic<void*> m_Draw3DAddress{ nullptr };
        std::atomic<bool> m_IsInstalled{ false };
        std::mutex m_Mutex{};

        // return: True if the pass was hooked.
        auto InstallOne(const char* name, const Signature& prologue, std::uint32_t rva, void* hooked, void** original,
            std::atomic<void*>& address) -> bool;

        // Disables the hook at the address. Does nothing for a null address (a pass that was not hooked).
        auto Disable(const std::atomic<void*>& address) -> void;

        auto Remove(std::atomic<void*>& address, const char* name) -> void;

        static auto __fastcall HookedDraw(std::uint32_t user) -> void;
        static auto __fastcall HookedDrawLate(std::uint32_t user) -> void;
        static auto __fastcall HookedDraw3D(std::uint32_t user) -> void;
    };
}