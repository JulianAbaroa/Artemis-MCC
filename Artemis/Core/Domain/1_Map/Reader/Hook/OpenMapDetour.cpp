module;

#include <windows.h>

module Map.Reader.Hook;
import :OpenMap;

import Platform.Memory.Type;
import Platform.Hook.System;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;
}

namespace Map::Reader::Hook
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

        if (relativePath.contains("campaign"))
        {
            s_Instance->m_FileStore.
                SetCampaignFilePath(fullPath.string());
            return;
        }

        if (relativePath.contains("shared"))
        {
            s_Instance->m_FileStore.
                SetSharedFilePath(fullPath.string());
            return;
        }

        std::string mapPath = fullPath.string();

        s_Instance->m_MapLoaderService.LoadMap(mapPath);
    }

    auto OpenMapDetour::Install() -> void
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        void* functionAddress = reinterpret_cast<void*>(
            m_AOBService.FindPattern(Signature::BlamOpenMap));
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedOpenMap),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[OpenMapDetour]", m_LogsService))
        {
            return;
        }

        m_IsHookInstalled.store(true);
    }

    auto OpenMapDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        Platform::Hook::System::WaitForDrain(s_InFlight, std::chrono::milliseconds(500));
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[OpenMapDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}