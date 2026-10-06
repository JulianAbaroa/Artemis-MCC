module;

#include <d3d11.h>
#include <dxgi.h>

export module Platform.Render.Hook:ResizeBuffers;

import Service.Logs.System;
import Platform.Render.Type;
import Platform.Render.State;
import Platform.Render.System;
import std;

export namespace Platform::Render::Hook
{
    // Hooks IDXGISwapChain::ResizeBuffers to release the render targets before the resize and rebind them after.
    class ResizeBuffersDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using ResizeBuffersFunction = Platform::Render::Type::ResizeBuffersFunction;

        using RenderStore = Platform::Render::State::RenderStore;

        using RenderService = Platform::Render::System::RenderService;
        using SwapChainLocator = Platform::Render::System::SwapChainLocator;

    public:
        ResizeBuffersDetour(LogsService& logsService, RenderStore& renderStore,
            RenderService& renderService, SwapChainLocator& swapChainLocator) :
            m_LogsService(logsService), m_RenderStore(renderStore),
            m_RenderService(renderService), m_SwapChainLocator(swapChainLocator) {}
        ~ResizeBuffersDetour() = default;

        // Locates ResizeBuffers and installs the hook. Does nothing if already installed.
        // return: False if the hook could not be installed.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        RenderStore& m_RenderStore;
        RenderService& m_RenderService;
        SwapChainLocator& m_SwapChainLocator;

        static inline ResizeBuffersDetour* s_Instance{ nullptr };
        static inline ResizeBuffersFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __stdcall HookedResizeBuffers(IDXGISwapChain* swapChain,
            UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat,
            UINT swapChainFlags) -> HRESULT;
    };
}