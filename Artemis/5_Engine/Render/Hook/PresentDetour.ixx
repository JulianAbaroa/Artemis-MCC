module;

#include <d3d11.h>
#include <dxgi.h>

export module Engine.Render.Hook:Present;

import Common.Log.System;
import Services.Telemetry.State;
import Engine.Render.Type;
import Engine.Render.State;
import Engine.Render.System;
import std;

export namespace Engine::Render::Hook
{
    // Hooks IDXGISwapChain::Present to run the render frame before the game presents it.
    class PresentDetour
    {
    private:
        using Log = Common::Log::System::Log;
        using TelemetryStore = Services::Telemetry::State::TelemetryStore;

        using PresentFunction = Engine::Render::Type::PresentFunction;

        using RenderStore = Engine::Render::State::RenderStore;

        using RenderService = Engine::Render::System::RenderService;
        using SwapChainLocator = Engine::Render::System::SwapChainLocator;

    public:
        PresentDetour(Log& log, TelemetryStore& telemetryStore,
            RenderStore& renderStore, RenderService& renderService,
            SwapChainLocator& swapChainLocator) : m_Log(log),
            m_TelemetryStore(telemetryStore), m_RenderStore(renderStore),
            m_RenderService(renderService), m_SwapChainLocator(swapChainLocator) {}
        ~PresentDetour() = default;

        // Locates Present and installs the hook. Does nothing if already installed.
        // return: False if the hook could not be installed.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        TelemetryStore& m_TelemetryStore;
        RenderStore& m_RenderStore;
        RenderService& m_RenderService;
        SwapChainLocator& m_SwapChainLocator;

        static inline PresentDetour* s_Instance{ nullptr };
        static inline PresentFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __stdcall HookedPresent(IDXGISwapChain* swapChain,
            UINT syncInterval, UINT flags) -> HRESULT;
    };
}