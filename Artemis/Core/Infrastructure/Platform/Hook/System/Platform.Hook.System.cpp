module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Hook.System;

import Platform.Hook.Type;

namespace
{
    using Common::Log::System::Log;

    using Platform::Hook::Type::k_TlsArrayOffset;
    using Platform::Hook::Type::k_MaxTlsIndex;
    using Platform::Hook::Type::k_TlsIndexRelOffset;
    using Platform::Hook::Type::k_TlsIndexNextOffset;

    // Copies bytes over code, restoring the page protection after.
    // return: False if the page cannot be made writable.
    auto WriteCode(void* address, const std::uint8_t* data, std::size_t size) -> bool
    {
        DWORD oldProtection{ 0 };
        if (!VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtection)) return false;

        std::memcpy(address, data, size);

        DWORD unusedProtection{ 0 };
        VirtualProtect(address, size, oldProtection, &unusedProtection);
        FlushInstructionCache(GetCurrentProcess(), address, size);

        return true;
    }
}

namespace Platform::Hook::System
{
    auto InstallDetour(void* functionAddress, void* hookedFunction,
        void** outOriginal, const char* tag, Log& log) -> bool
    {
        if (!functionAddress)
        {
            log.Message("{} ERROR: Failed to obtain the function address.", tag);
            return false;
        }

        MH_RemoveHook(functionAddress);

        if (MH_CreateHook(functionAddress, hookedFunction,
            reinterpret_cast<LPVOID*>(outOriginal)) != MH_OK)
        {
            log.Message("{} ERROR: Failed to create the hook.", tag);
            return false;
        }

        if (MH_EnableHook(functionAddress) != MH_OK)
        {
            log.Message("{} ERROR: Failed to enable the hook.", tag);
            MH_RemoveHook(functionAddress);
            return false;
        }

        log.Message("{} INFO: Hook installed, address: {:016X}", tag, reinterpret_cast<uintptr_t>(functionAddress));
        return true;
    }

    auto DisableDetour(void* functionAddress) -> void
    {
        MH_DisableHook(functionAddress);
    }

    auto RemoveDetour(void* functionAddress, const char* tag,
        Log& log) -> void
    {
        MH_RemoveHook(functionAddress);
        log.Message("{} INFO: Hook uninstalled.", tag);
    }

    auto WritePatch(void* address, std::span<const std::uint8_t> bytes,
        std::vector<std::uint8_t>& outOriginal, const char* tag, Log& log) -> bool
    {
        if (!address)
        {
            log.Message("{} ERROR: Failed to obtain the patch address.", tag);
            return false;
        }

        const auto* current{ static_cast<const std::uint8_t*>(address) };
        outOriginal.assign(current, current + bytes.size());

        if (!WriteCode(address, bytes.data(), bytes.size()))
        {
            log.Message("{} ERROR: Failed to make the code writable.", tag);
            outOriginal.clear();
            return false;
        }

        log.Message("{} INFO: Patch written, address: {:016X}.", tag, reinterpret_cast<std::uintptr_t>(address));
        return true;
    }

    auto RestorePatch(void* address, std::span<const std::uint8_t> original,
        const char* tag, Log& log) -> void
    {
        if (!address || original.empty()) return;

        if (!WriteCode(address, original.data(), original.size()))
        {
            log.Message("{} ERROR: Failed to restore the patch.", tag);
            return;
        }

        log.Message("{} INFO: Patch restored.", tag);
    }

    auto ResolveRelative(std::uintptr_t match, std::uintptr_t relOffset, std::uintptr_t nextOffset) -> std::uintptr_t
    {
        std::int32_t relative{ 0 };
        std::memcpy(&relative, reinterpret_cast<const void*>(match + relOffset), sizeof(relative));

        return static_cast<std::uintptr_t>(static_cast<std::intptr_t>(match + nextOffset) + relative);
    }

    auto ResolveTlsIndex(std::uintptr_t match) -> const std::uint32_t*
    {
        if (match == 0) return nullptr;

        return reinterpret_cast<const std::uint32_t*>(ResolveRelative(match, k_TlsIndexRelOffset, k_TlsIndexNextOffset));
    }

    auto ReadThreadContext(std::uint32_t tlsIndex) -> std::uintptr_t
    {
        if (tlsIndex > k_MaxTlsIndex) return 0;

        const auto tlsArray{ static_cast<std::uintptr_t>(__readgsqword(static_cast<unsigned long>(k_TlsArrayOffset))) };
        return *reinterpret_cast<const std::uintptr_t*>(tlsArray + (tlsIndex * sizeof(std::uintptr_t)));
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