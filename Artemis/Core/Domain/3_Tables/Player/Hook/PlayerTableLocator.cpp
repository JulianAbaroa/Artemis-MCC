module;

#include <windows.h>

module Tables.Player.Hook;
import :PlayerTable;

import Platform.Memory.Type;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;
}

namespace Tables::Player::Hook
{
    auto PlayerTableLocator::FindAndStoreTableBase() -> void
    {
        std::uintptr_t tableBase = m_PlayerStore.GetBase();
        if (tableBase == 0)
        {
            tableBase = this->GetPlayerTable();
            if (!tableBase)
            {
                m_LogsService.Message("[PlayerTableLocator] ERROR:"
                    " PlayerTableBase invalid.");
                return;
            }

            m_LogsService.Message("[PlayerTableLocator] INFO: PlayerTable:"
                " {:016X}", tableBase);

            m_PlayerStore.SetBase(tableBase);
        }
    }

    // Returns the pointer to the PlayerTable within the game's memory.
    // This table acts as the primary data structure for session-specific player data,
    // including nicknames, service IDs, and real-time state: world position, rotation,
    // look vector, and handles for primary/secondary weapons, biped, and objective items.
    // Note: this function relies on Thread Local Storage (TLS) to retrieve the correct
    // telemetry context for the current execution thread.
    // Reference: See "Core/Common/Types/BlamTypes.h" to find the specific structs.
    auto PlayerTableLocator::GetPlayerTable() -> std::uintptr_t
    {
        __try
        {
            std::uintptr_t tlsArray = (std::uintptr_t)__readgsqword(0x58);

            std::uintptr_t match = m_AOBService.FindPattern(
                Signature::TelemetryIdModifier);

            if (match)
            {
                std::int32_t relativeOffset = *(std::int32_t*)(match + 2);
                std::uintptr_t telemetryIdAddr = (match + 6) + relativeOffset;
                std::uint32_t telemetryIdx = *(std::uint32_t*)(telemetryIdAddr);

                if (telemetryIdx <= 1000)
                {
                    std::uintptr_t threadContext = *(std::uintptr_t*)(tlsArray + 
                            (static_cast<unsigned long long>(telemetryIdx) * 8));

                    if (threadContext)
                    {
                        std::uintptr_t telemetryData = 
                            *(std::uintptr_t*)(threadContext + 0x18);

                        if (telemetryData)
                        {
                            std::uintptr_t playerTable = 
                                *(std::uintptr_t*)(telemetryData + 0x50);

                            return playerTable;
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