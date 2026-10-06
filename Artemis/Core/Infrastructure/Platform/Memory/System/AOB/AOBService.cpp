module;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

module Platform.Memory.System;
import :AOB;

namespace
{
    // return: One int per byte of the pattern, -1 for a wildcard.
    auto ParsePattern(std::string_view pattern) -> std::vector<int>
    {
        std::vector<int> bytes{};
        std::size_t index = 0;

        while (index < pattern.size())
        {
            if (pattern[index] == ' ')
            {
                ++index;
                continue;
            }

            if (pattern[index] == '?')
            {
                bytes.push_back(-1);
                ++index;

                if (index < pattern.size() && pattern[index] == '?') ++index;

                continue;
            }

            int value = 0;
            auto [end, error] = std::from_chars(pattern.data() + index, pattern.data() + pattern.size(), value, 16);

            if (error == std::errc{})
            {
                bytes.push_back(value);
                index = static_cast<std::size_t>(end - pattern.data());
            }
            else
            {
                ++index;
            }
        }

        return bytes;
    }
}

namespace Platform::Memory::System
{
    auto AOBService::FindPattern(const Signature& signature, const wchar_t* moduleName) -> std::uintptr_t
    {
        return this->FindPattern(signature.Pattern, moduleName, signature.Name);
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

        const std::uintptr_t address = this->Scan(baseAddress, sizeOfImage, pattern, name);

        if (address == 0) m_LogsService.Message("[AOBService] ERROR:"
            " Pattern '{}' not found.", name);

        return address;
    }

    auto AOBService::Scan(std::uintptr_t base, std::size_t size, const char* pattern,
        const char* name) -> std::uintptr_t
    {
        auto patternBytes = ParsePattern(pattern);
        if (patternBytes.empty()) return 0;

        const auto* scanStart = reinterpret_cast<const std::uint8_t*>(base);
        std::size_t patternSize = patternBytes.size();
        const int* patternData = patternBytes.data();

        for (std::size_t i = 0; i <= size - patternSize; ++i)
        {
            bool isFound = true;

            for (std::size_t j = 0; j < patternSize; ++j)
            {
                if (patternData[j] != -1 && scanStart[i + j] != static_cast<std::uint8_t>(patternData[j]))
                {
                    isFound = false;
                    break;
                }
            }

            if (isFound) return reinterpret_cast<std::uintptr_t>(&scanStart[i]);
        }

        return 0;
    }
}