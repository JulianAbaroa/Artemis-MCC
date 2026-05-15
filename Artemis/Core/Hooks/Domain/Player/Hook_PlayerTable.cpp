#include "pch.h"

// Header.
#include "Hook_PlayerTable.h"

// Types.
#include "Core/Types/Infrastructure/AOB/Signatures.h"

// --- States ---

#include "Core/States/Domain/Player/State_PlayerTable.h"

// --- Systems ---

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

void Hook_PlayerTable::FindAndStoreTableBase()
{
    uintptr_t tableBase = m_Deps.State_PlayerTable.GetPlayerTableBase();
    if (tableBase == 0)
    {
        tableBase = this->GetPlayerTable();
        if (!tableBase)
        {
            m_Deps.System_Debug.Log("[PlayerTableSystem] ERROR:"
                " PlayerTableBase invalid.");
            return;
        }

        m_Deps.System_Debug.Log("[PlayerTableSystem] INFO: PlayerTable: 0x%llX", tableBase);
        m_Deps.State_PlayerTable.SetPlayerTableBase(tableBase);
    }
}

// Returns the pointer to the PlayerTable within the game's memory.
// This table acts as the primary data structure for session-specific player data,
// including nicknames, service IDs, and real-time state: world position, rotation,
// look vector, and handles for primary/secondary weapons, biped, and objective items.
// Note: this function relies on Thread Local Storage (TLS) to retrieve the correct
// telemetry context for the current execution thread.
// Reference: See "Core/Common/Types/BlamTypes.h" to find the specific structs.
uintptr_t Hook_PlayerTable::GetPlayerTable()
{
    __try
    {
        uintptr_t tlsArray = (uintptr_t)__readgsqword(0x58);

        uintptr_t match = m_Deps.System_AOBScanner.
            FindPattern(Signatures::TelemetryIdModifier);

        if (match)
        {
            int32_t relativeOffset = *(int32_t*)(match + 2);
            uintptr_t telemetryIdAddr = (match + 6) + relativeOffset;
            uint32_t telemetryIdx = *(uint32_t*)(telemetryIdAddr);

            if (telemetryIdx <= 1000)
            {
                uintptr_t threadContext = *(uintptr_t*)(tlsArray + (static_cast<unsigned long long>(telemetryIdx) * 8));

                if (threadContext)
                {
                    uintptr_t telemetryData = *(uintptr_t*)(threadContext + 0x18);

                    if (telemetryData)
                    {
                        uintptr_t playerTable = *(uintptr_t*)(telemetryData + 0x50);
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