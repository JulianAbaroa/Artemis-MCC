module;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

module Platform.Memory.System;
import :AOB;

namespace Platform::Memory::System
{
    auto AOBService::FindPattern(const Signature& sig, const wchar_t* moduleName) -> std::uintptr_t
    {
        return this->FindPattern(sig.pattern, moduleName, sig.name);
    }

    auto AOBService::FindPattern(const char* pattern, const wchar_t* moduleName, const char* name) -> std::uintptr_t
    {
        HMODULE hModule = GetModuleHandle(moduleName);
        if (!hModule) return 0;

        auto baseAddress = reinterpret_cast<std::uintptr_t>(hModule);

        auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(baseAddress);
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;

        auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(baseAddress + dosHeader->e_lfanew);
        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return 0;

        std::size_t sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;

        return this->Scan(baseAddress, sizeOfImage, pattern, name);
    }

    auto AOBService::Scan(std::uintptr_t base, std::size_t size, const char* pattern,
        const char* name) -> std::uintptr_t
    {
        auto patternToByte = [](std::string_view pat) {
            auto bytes = std::vector<int>{};
            std::size_t i = 0;

            while (i < pat.size())
            {
                if (pat[i] == ' ')
                {
                    ++i;
                    continue;
                }

                if (pat[i] == '?')
                {
                    bytes.push_back(-1);
                    ++i;
                    if (i < pat.size() && pat[i] == '?') ++i;
                    continue;
                }

                int value = 0;
                auto [ptr, ec] = std::from_chars(pat.data() + i, pat.data() + pat.size(), value, 16);
                if (ec == std::errc{})
                {
                    bytes.push_back(value);
                    i = static_cast<std::size_t>(ptr - pat.data());
                }
                else
                {
                    ++i;
                }
            }
            return bytes;
            };

        auto patternBytes = patternToByte(pattern);
        if (patternBytes.empty()) return 0;

        const auto* scanStart = reinterpret_cast<const std::uint8_t*>(base);
        std::size_t patternSize = patternBytes.size();
        const int* patternData = patternBytes.data();

        for (std::size_t i = 0; i <= size - patternSize; ++i)
        {
            bool found = true;
            for (std::size_t j = 0; j < patternSize; ++j)
            {
                if (patternData[j] != -1 && scanStart[i + j] != static_cast<std::uint8_t>(patternData[j]))
                {
                    found = false;
                    break;
                }
            }

            if (found)
            {
                return reinterpret_cast<std::uintptr_t>(&scanStart[i]);
            }
        }

        return 0;
    }
}