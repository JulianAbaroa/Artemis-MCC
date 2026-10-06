module;

#include <d3d11.h>
#include <dxgi.h>

module Platform.Render.Hook;
import :Present;

import Platform.Hook.System;
import std;

namespace
{
    using std::chrono::milliseconds;

    constexpr milliseconds k_IdleTimeout{ 1000 };
}

namespace Platform::Render::Hook
{
    auto __stdcall PresentDetour::HookedPresent(IDXGISwapChain* swapChain,
        UINT syncInterval, UINT flags) -> HRESULT
    {
        auto* self = s_Instance;

        if (self)
        {
            RenderStore::HookScope scope{ self->m_RenderStore };

            self->m_TelemetryStore.RecordPresent();
            self->m_RenderService.PresentFrame(swapChain);
        }

        return s_OriginalFunction(swapChain, syncInterval, flags);
    }

    auto PresentDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress = m_SwapChainLocator.Locate().Present;
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedPresent),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[PresentDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto PresentDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());

        if (!m_RenderService.WaitForIdle(k_IdleTimeout))
        {
            m_LogsService.Message("[PresentDetour] WARNING:"
                " A call was still running when the hook was removed.");
        }

        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[PresentDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}