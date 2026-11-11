export module Engine.Base.Memory.System:AOB;

import Common.Log.System;
import Engine.Base.Memory.Type;
import std;

export namespace Engine::Base::Memory::System
{
    // Finds byte patterns (AOB) inside a loaded module.
    class AOBService
    {
    private:
        using Log = Common::Log::System::Log;

        using Signature = Engine::Base::Memory::Type::Signature::Signature;

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

        // Finds a function by its RVA when only the start of its prologue is known.
        // param anchor: Pattern of the first bytes of the prologue.
        // param window: How many bytes after the RVA the pattern may begin.
        // return: Address of the function (module base + rva), or 0 if the module is not loaded, the pattern does not begin within the
        // window or the unwind table of the module does not say a function begins exactly at the rva (the game was updated). The cause is
        // logged.
        auto FindFunctionAtRva(const Signature& anchor, std::uint32_t rva, std::uint32_t window,
            const wchar_t* moduleName = L"haloreach.dll") -> std::uintptr_t;

    private:
        Log& m_Log;

        // return: Whether the module is loaded. Gives its base and the size of its image.
        auto LocateModule(const wchar_t* moduleName, std::uintptr_t& base, std::size_t& size) const -> bool;

        auto Scan(std::uintptr_t base, std::size_t size, const char* pattern,
            const char* name) -> std::uintptr_t;
    };
}