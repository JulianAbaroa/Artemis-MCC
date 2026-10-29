module;

#include <d3d11.h>
#include <dxgi.h>

export module Engine.Render.Hook:ResizeBuffers;

import Common.Log.System;
import Engine.Render.Type;
import Engine.Render.State;
import Engine.Render.System;
import std;

export namespace Engine::Render::Hook
{
    // Hooks IDXGISwapChain::ResizeBuffers to release the render targets before the resize and rebind them after.
    class ResizeBuffersDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using ResizeBuffersFunction = Engine::Render::Type::ResizeBuffersFunction;

        using RenderStore = Engine::Render::State::RenderStore;

        using RenderService = Engine::Render::System::RenderService;
        using SwapChainLocator = Engine::Render::System::SwapChainLocator;

    public:
        ResizeBuffersDetour(Log& log, RenderStore& renderStore,
            RenderService& renderService, SwapChainLocator& swapChainLocator) :
            m_Log(log), m_RenderStore(renderStore),
            m_RenderService(renderService), m_SwapChainLocator(swapChainLocator) {}
        ~ResizeBuffersDetour() = default;

        // Locates ResizeBuffers and installs the hook. Does nothing if already installed.
        // return: False if the hook could not be installed.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
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