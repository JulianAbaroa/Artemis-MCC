export module Platform.Memory.System:AOB;

import Common.Log.System;
import Platform.Memory.Type;
import std;

export namespace Platform::Memory::System
{
    // Finds byte patterns (AOB) inside a loaded module.
    class AOBService
    {
    private:
        using Log = Common::Log::System::Log;

        using Signature = Platform::Memory::Type::Signature::Signature;

    public:
        AOBService(Log& log) : m_Log(log) {}
        ~AOBService() = default;

        // return: Address of the first match, or 0 if the module or the pattern is not found.
        auto FindPattern(const Signature& signature,
            const wchar_t* moduleName = L"haloreach.dll") -> std::uintptr_t;

        // param pattern: Hex bytes separated by spaces, "?" or "??" match any byte.
        // return: Address of the first match, or 0 if the module or the pattern is not found.
        auto FindPattern(const char* pattern, const wchar_t* moduleName = L"haloreach.dll",
            const char* name = "Unknown") -> std::uintptr_t;

    private:
        Log& m_Log;

        auto Scan(std::uintptr_t base, std::size_t size, const char* pattern,
            const char* name) -> std::uintptr_t;
    };
}