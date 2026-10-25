module;

#include <windows.h>

module Platform.Lifecycle.Hook;
import :OpenMap;

import Platform.Memory.Type;
import Platform.Hook.System;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Hook::System::k_DrainTimeout;
}

namespace Platform::Lifecycle::Hook
{
    auto __fastcall OpenMapDetour::HookedOpenMap(std::uint64_t param1,
        std::uint64_t param2, std::uint64_t mapRelativePath, std::uint32_t* param4) -> void
    {
        Platform::Hook::System::InFlightScope scope(s_InFlight);

        s_OriginalFunction(param1, param2, mapRelativePath, param4);

        std::string relativePath =
            reinterpret_cast<const char*>(mapRelativePath);

        char exePath[MAX_PATH]{};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);

        std::filesystem::path gameRoot =
            std::filesystem::path(exePath).parent_path().
            parent_path().parent_path().parent_path();

        std::filesystem::path fullPath = gameRoot / relativePath;

        s_Instance->m_LifecycleService.RaiseMapFileOpened(fullPath.string());
    }

    auto OpenMapDetour::Install() -> void
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        void* functionAddress = reinterpret_cast<void*>(
            m_AOBService.FindPattern(Signature::k_BlamOpenMap));
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedOpenMap),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[OpenMapDetour]", m_Log))
        {
            return;
        }

        m_IsHookInstalled.store(true);
    }

    auto OpenMapDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[OpenMapDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}