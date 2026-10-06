module;

#include <d3d11.h>
#include <dxgi.h>

export module Platform.Render.Hook:Present;

import Service.Logs.System;
import Service.Telemetry.State;
import Platform.Render.Type;
import Platform.Render.State;
import Platform.Render.System;
import std;

export namespace Platform::Render::Hook
{
    // Hooks IDXGISwapChain::Present to run the render frame before the game presents it.
    class PresentDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using TelemetryStore = Service::Telemetry::State::TelemetryStore;

        using PresentFunction = Platform::Render::Type::PresentFunction;

        using RenderStore = Platform::Render::State::RenderStore;

        using RenderService = Platform::Render::System::RenderService;
        using SwapChainLocator = Platform::Render::System::SwapChainLocator;

    public:
        PresentDetour(LogsService& logsService, TelemetryStore& telemetryStore,
            RenderStore& renderStore, RenderService& renderService,
            SwapChainLocator& swapChainLocator) : m_LogsService(logsService),
            m_TelemetryStore(telemetryStore), m_RenderStore(renderStore),
            m_RenderService(renderService), m_SwapChainLocator(swapChainLocator) {}
        ~PresentDetour() = default;

        // Locates Present and installs the hook. Does nothing if already installed.
        // return: False if the hook could not be installed.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
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