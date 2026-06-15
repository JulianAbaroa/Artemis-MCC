#include "pch.h"

#include "Hook_ObjectTable.h"

#include "Core/Types/Other/Memory/AOB/Signatures.h"

#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"

#include "Core/Systems/Other/Memory/AOB/System_AOBScanner.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

void Hook_ObjectTable::FindAndStoreTableBase()
{
    uintptr_t tableBase = m_Deps.State_ObjectTable.GetBase();
    if (tableBase == 0)
    {
        tableBase = this->GetObjectTable();
        if (!tableBase)
        {
            m_Deps.System_Logs.Log("[ObjectTable] ERROR:"
                " ObjectTableBase invalid.");
            return;
        }

        m_Deps.System_Logs.Log("[ObjectTable] INFO: ObjectTable:"
            " 0x%llX", tableBase);

        m_Deps.State_ObjectTable.SetBase(tableBase);
    }
}

// Returns the pointer to the Global Object Table (Indirection Table).
// This table acts as a lookup array where each 24-byte (0x18) entry maps a 
// networked 'Handle' to a physical memory address.
// Resolution logic: The lower 16 bits of a Handle (0xFFFF) serve as the index
// into this table, while the upper 16 bits function as a 'Salt' for versioning.
// Each entry contains a 64-bit pointer to the actual entity data (Bipeds, Weapons, etc.).
// Capacity: Probably 1024 slots, supporting the 650 Forge-item limit
// plus dynamic entities like projectiles and player assets.
uintptr_t Hook_ObjectTable::GetObjectTable()
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
                    uintptr_t telemetryData = *(uintptr_t*)(threadContext + 0x10);

                    if (telemetryData) 
                    {
                        uintptr_t objectTable = *(uintptr_t*)(telemetryData + 0x50);
                        return objectTable;
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