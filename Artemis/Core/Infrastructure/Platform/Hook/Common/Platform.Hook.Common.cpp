module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Hook.Common;

namespace Platform::Hook::Common
{
    auto InstallDetour(void* functionAddress, void* hookedFunction,
        void** outOriginal, const char* tag, LogsService& logsService) -> bool
    {
        if (!functionAddress)
        {
            logsService.Message("{} ERROR: Failed to obtain the function address.", tag);
            return false;
        }

        MH_RemoveHook(functionAddress);

        if (MH_CreateHook(functionAddress, hookedFunction,
            reinterpret_cast<LPVOID*>(outOriginal)) != MH_OK)
        {
            logsService.Message("{} ERROR: Failed to create the hook.", tag);
            return false;
        }

        if (MH_EnableHook(functionAddress) != MH_OK)
        {
            logsService.Message("{} ERROR: Failed to enable the hook.", tag);
            MH_RemoveHook(functionAddress);
            return false;
        }

        logsService.Message("{} INFO: Hook installed, address: {:016X}", tag, reinterpret_cast<uintptr_t>(functionAddress));
        return true;
    }

    auto DisableDetour(void* functionAddress) -> void
    {
        MH_DisableHook(functionAddress);
    }

    auto RemoveDetour(void* functionAddress, const char* tag,
        LogsService& logsService) -> void
    {
        MH_RemoveHook(functionAddress);
        logsService.Message("{} INFO: Hook uninstalled.", tag);
    }

    auto UninstallDetour(void* functionAddress, const char* tag,
        LogsService& logsService) -> void
    {
        DisableDetour(functionAddress);
        RemoveDetour(functionAddress, tag, logsService);
    }

    auto WaitForDrain(std::atomic<int>& counter,
        std::chrono::milliseconds timeout) -> bool
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;

        while (counter.load(std::memory_order_acquire) > 0 &&
            std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        return counter.load(std::memory_order_acquire) == 0;
    }
}