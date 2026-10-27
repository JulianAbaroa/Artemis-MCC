module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Window.Hook;
import :GetRawInputData;

import Platform.Window.Type;
import Platform.Hook.System;
import std;

namespace
{
    using Platform::Window::Type::RawMouse;

    using Platform::Hook::System::k_DrainTimeout;

    inline auto SwallowMouse(RAWMOUSE& mouse) -> void
    {
        mouse.lLastX = 0;
        mouse.lLastY = 0;
        mouse.ulButtons = 0;
    }
}

namespace Platform::Window::Hook
{
    auto WINAPI GetRawInputDataDetour::HookedGetRawInputData(HRAWINPUT hRawInput,
        UINT command, LPVOID data, PUINT dataSize, UINT headerSize) -> UINT
    {
        auto* self = s_Instance;

        const UINT size = s_OriginalFunction(
            hRawInput, command, data, dataSize, headerSize);

        if (!self) return size;

        Platform::Hook::System::InFlightScope scope(s_InFlight);

        if (size == static_cast<UINT>(-1) || !data || command != RID_INPUT) return size;

        auto* raw = static_cast<RAWINPUT*>(data);
        if (raw->header.dwType != RIM_TYPEMOUSE) return size;

        RAWMOUSE& mouse = raw->data.mouse;

        if (self->m_SettingsStore.IsMenuVisible())
        {
            if (self->m_SettingsStore.IsMouseFreezeEnabled()) SwallowMouse(mouse);
            return size;
        }

        if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) return size;

        RawMouse input{};
        input.DeltaX = mouse.lLastX;
        input.DeltaY = mouse.lLastY;
        input.ButtonFlags = mouse.usButtonFlags;
        input.ButtonData = mouse.usButtonData;

        if (self->m_InputService.RaiseRawMouse(input)) SwallowMouse(mouse);

        return size;
    }

    auto GetRawInputDataDetour::Install() -> void
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (!user32)
        {
            m_Log.Message("[GetRawInputDataDetour] ERROR:"
                " Could not get the handle of user32.dll.");
            return;
        }

        void* functionAddress = reinterpret_cast<void*>(
            GetProcAddress(user32, "GetRawInputData"));
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedGetRawInputData),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[GetRawInputDataDetour]", m_Log))
        {
            return;
        }

        m_IsHookInstalled.store(true);
    }

    auto GetRawInputDataDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[GetRawInputDataDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}