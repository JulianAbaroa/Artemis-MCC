module;

#include <d3d11.h>
#include <dxgi.h>

module Engine.Render.Hook;
import :ResizeBuffers;

import Engine.Base.Hook.System;
import Engine.Render.Type;
import std;

namespace
{
    using std::chrono::milliseconds;

    using Engine::Render::Type::k_IdleTimeout;
}

namespace Engine::Render::Hook
{
    auto __stdcall ResizeBuffersDetour::HookedResizeBuffers(IDXGISwapChain* swapChain,
        UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat,
        UINT swapChainFlags) -> HRESULT
    {
        auto* self = s_Instance;

        if (!self)
        {
            return s_OriginalFunction(swapChain, bufferCount,
                width, height, newFormat, swapChainFlags);
        }

        RenderStore::HookScope scope{ self->m_RenderStore };

        self->m_RenderService.BeginResize();

        const HRESULT result = s_OriginalFunction(swapChain, bufferCount,
            width, height, newFormat, swapChainFlags);

        self->m_RenderService.EndResize(swapChain, result);

        return result;
    }

    auto ResizeBuffersDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress = m_SwapChainLocator.Locate().ResizeBuffers;
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedResizeBuffers),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[ResizeBuffersDetour]", m_Log))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto ResizeBuffersDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());

        if (!m_RenderService.WaitForIdle(k_IdleTimeout))
        {
            m_Log.Message("[ResizeBuffersDetour] WARNING:"
                " A call was still running when the hook was removed.");
        }

        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[ResizeBuffersDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}