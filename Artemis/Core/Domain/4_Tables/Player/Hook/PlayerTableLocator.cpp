module;

#include <windows.h>

module Tables.Player.Hook;

import Platform.Memory.Type;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    // Sanity limit of the telemetry index. Anything above it is not a valid thread slot.
    constexpr std::uint32_t k_MaxTelemetryId{ 1000 };

    // Offset of the telemetry data pointer inside the thread context.
    constexpr std::uintptr_t k_TelemetryDataOffset{ 0x18 };

    // Offset of the player table pointer inside the telemetry data.
    constexpr std::uintptr_t k_PlayerTableOffset{ 0x50 };

    // Offset of the player mapping globals pointer inside the thread context (A9_SplitScreen.txt sec. 11.1).
    constexpr std::uintptr_t k_MappingOffset{ 0x158 };
}

namespace Tables::Player::Hook
{
    auto PlayerTableLocator::FindAndStoreTableBase() -> void
    {
        const bool hasTable{ m_PlayerStore.GetBase() != 0 };
        const bool hasMapping{ m_PlayerStore.GetMappingBase() != 0 };
        if (hasTable && hasMapping) return;

        const std::uintptr_t threadContext{ this->GetThreadContext() };

        if (!hasTable)
        {
            const std::uintptr_t tableBase{ this->GetPlayerTable(threadContext) };
            if (!tableBase)
            {
                m_LogsService.Message("[PlayerTableLocator] ERROR:"
                    " Failed to find the player table.");
            }
            else
            {
                m_LogsService.Message("[PlayerTableLocator] INFO:"
                    " Player table found at {:016X}.", tableBase);

                m_PlayerStore.SetBase(tableBase);
            }
        }

        if (!hasMapping)
        {
            const std::uintptr_t mappingBase{ this->GetMapping(threadContext) };
            if (!mappingBase)
            {
                m_LogsService.Message("[PlayerTableLocator] ERROR:"
                    " Failed to find the player mapping.");
            }
            else
            {
                m_LogsService.Message("[PlayerTableLocator] INFO:"
                    " Player mapping found at {:016X}.", mappingBase);

                m_PlayerStore.SetMappingBase(mappingBase);
            }
        }
    }

    auto PlayerTableLocator::GetThreadContext() -> std::uintptr_t
    {
        __try
        {
            // note: gs:[0x58] is the thread local storage array of the current thread.
            std::uintptr_t tlsArray{ (std::uintptr_t)__readgsqword(0x58) };

            std::uintptr_t match{ m_AOBService.FindPattern(
                Signature::TelemetryIdModifier) };

            if (!match) return 0;

            // note: The match is a "mov ecx, [rip+rel32]". The rel32 is at +2 and the instruction is 6 bytes long.
            std::int32_t relativeOffset{ *(std::int32_t*)(match + 2) };
            std::uintptr_t telemetryIdAddr{ (match + 6) + relativeOffset };
            std::uint32_t telemetryIdx{ *(std::uint32_t*)(telemetryIdAddr) };

            if (telemetryIdx > k_MaxTelemetryId) return 0;

            return *(std::uintptr_t*)(tlsArray + (static_cast<unsigned long long>(telemetryIdx) * 8));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    auto PlayerTableLocator::GetPlayerTable(std::uintptr_t threadContext) -> std::uintptr_t
    {
        if (!threadContext) return 0;

        __try
        {
            std::uintptr_t telemetryData{ *(std::uintptr_t*)(threadContext + k_TelemetryDataOffset) };
            if (!telemetryData) return 0;

            return *(std::uintptr_t*)(telemetryData + k_PlayerTableOffset);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    auto PlayerTableLocator::GetMapping(std::uintptr_t threadContext) -> std::uintptr_t
    {
        if (!threadContext) return 0;

        __try
        {
            return *(std::uintptr_t*)(threadContext + k_MappingOffset);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }
}