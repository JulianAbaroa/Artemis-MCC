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
}

namespace Tables::Player::Hook
{
    auto PlayerTableLocator::FindAndStoreTableBase() -> void
    {
        std::uintptr_t tableBase{ m_PlayerStore.GetBase() };
        if (tableBase == 0)
        {
            tableBase = this->GetPlayerTable();
            if (!tableBase)
            {
                m_LogsService.Message("[PlayerTableLocator] ERROR:"
                    " Failed to find the player table.");
                return;
            }

            m_LogsService.Message("[PlayerTableLocator] INFO:"
                " Player table found at {:016X}.", tableBase);

            m_PlayerStore.SetBase(tableBase);
        }
    }

    auto PlayerTableLocator::GetPlayerTable() -> std::uintptr_t
    {
        __try
        {
            // note: gs:[0x58] is the thread local storage array of the current thread.
            std::uintptr_t tlsArray{ (std::uintptr_t)__readgsqword(0x58) };

            std::uintptr_t match{ m_AOBService.FindPattern(
                Signature::TelemetryIdModifier) };

            if (match)
            {
                // note: The match is a "mov ecx, [rip+rel32]". The rel32 is at +2 and the instruction is 6 bytes long.
                std::int32_t relativeOffset{ *(std::int32_t*)(match + 2) };
                std::uintptr_t telemetryIdAddr{ (match + 6) + relativeOffset };
                std::uint32_t telemetryIdx{ *(std::uint32_t*)(telemetryIdAddr) };

                if (telemetryIdx <= k_MaxTelemetryId)
                {
                    std::uintptr_t threadContext{ *(std::uintptr_t*)(tlsArray +
                        (static_cast<unsigned long long>(telemetryIdx) * 8)) };

                    if (threadContext)
                    {
                        std::uintptr_t telemetryData{
                            *(std::uintptr_t*)(threadContext + k_TelemetryDataOffset) };

                        if (telemetryData)
                        {
                            return *(std::uintptr_t*)(telemetryData + k_PlayerTableOffset);
                        }
                    }
                }
            }

            return 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }
}